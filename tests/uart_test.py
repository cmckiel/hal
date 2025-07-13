#!/usr/bin/env python3
"""
Comprehensive UART Integration Test Suite
Tests UART driver functionality through loopback and command-response patterns
"""
import serial
import time
import sys
import json
import random
import threading
from dataclasses import dataclass
from typing import List, Dict, Any, Optional, Tuple
from enum import Enum

class TestResult(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    SKIP = "SKIP"

@dataclass
class TestCase:
    name: str
    description: str
    result: TestResult
    details: str = ""
    execution_time: float = 0.0

class UARTTestSuite:
    def __init__(self, uart1_port='/dev/ttyUSB0', uart2_port='/dev/ttyACM0', baudrate=115200):
        self.uart1_port = uart1_port
        self.uart2_port = uart2_port
        self.baudrate = baudrate
        self.uart1 = None
        self.uart2 = None
        self.test_results = []
        self.command_mode = False

        # Test patterns for various scenarios
        self.test_patterns = {
            'ascii': b'Hello World!',
            'binary': bytes([0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD]),
            'special_chars': b'!@#$%^&*()_+-=[]{}|;:,.<>?',
            'newlines': b'Line1\nLine2\r\nLine3\r',
            'long_message': b'A' * 1000,
            'empty': b'',
            'single_byte': b'X',
        }

        # Command protocol constants
        self.CMD_PREFIX = b'CMD:'
        self.CMD_ENTER_TEST_MODE = b'CMD:ENTER_TEST_MODE\r\n'
        self.CMD_EXIT_TEST_MODE = b'CMD:EXIT_TEST_MODE\r\n'
        self.CMD_BAUDRATE_TEST = b'CMD:BAUDRATE_TEST:'
        self.CMD_ECHO_TEST = b'CMD:ECHO:'
        self.CMD_BUFFER_TEST = b'CMD:BUFFER_TEST:'
        self.CMD_TIMING_TEST = b'CMD:TIMING_TEST:'
        self.CMD_ERROR_TEST = b'CMD:ERROR_TEST:'

        self.RESPONSE_OK = b'OK\r\n'
        self.RESPONSE_ERROR = b'ERROR\r\n'
        self.RESPONSE_READY = b'READY\r\n'

    def setup_connections(self):
        """Initialize UART connections"""
        print("Setting up UART connections...")
        try:
            self.uart1 = serial.Serial(self.uart1_port, baudrate=self.baudrate, timeout=1)
            self.uart2 = serial.Serial(self.uart2_port, baudrate=self.baudrate, timeout=1)
            time.sleep(0.5)  # Allow connections to stabilize

            # Clear any existing data
            self.uart1.flush()
            self.uart2.flush()
            self.uart1.reset_input_buffer()
            self.uart2.reset_input_buffer()

            return True
        except Exception as e:
            print(f"Failed to setup connections: {e}")
            return False

    def cleanup_connections(self):
        """Clean up UART connections"""
        try:
            if self.uart1:
                self.uart1.close()
            if self.uart2:
                self.uart2.close()
            print("UART connections closed")
        except:
            pass

    def run_test_case(self, test_func, name, description):
        """Run a single test case and record results"""
        start_time = time.time()
        try:
            result, details = test_func()
            execution_time = time.time() - start_time

            test_case = TestCase(
                name=name,
                description=description,
                result=result,
                details=details,
                execution_time=execution_time
            )
            self.test_results.append(test_case)

            status_icon = "✅" if result == TestResult.PASS else "❌"
            print(f"{status_icon} {name}: {result.value} ({execution_time:.3f}s)")
            if details:
                print(f"   Details: {details}")

        except Exception as e:
            execution_time = time.time() - start_time
            test_case = TestCase(
                name=name,
                description=description,
                result=TestResult.FAIL,
                details=f"Exception: {str(e)}",
                execution_time=execution_time
            )
            self.test_results.append(test_case)
            print(f"❌ {name}: FAIL - Exception: {e}")

    # ===== LOOPBACK TESTS (Basic Smoke Tests) =====

    def test_basic_loopback(self):
        """Test basic UART loopback functionality"""
        test_message = b'Hello, is this the Krusty Krab?\r'

        # Send from UART1 to UART2
        self.uart1.write(test_message)
        time.sleep(0.1)
        received = self.uart2.read(len(test_message))

        if received == test_message:
            return TestResult.PASS, f"Loopback successful: {test_message}"
        else:
            return TestResult.FAIL, f"Expected: {test_message}, Got: {received}"

    def test_bidirectional_loopback(self):
        """Test bidirectional loopback"""
        msg1 = b'UART1->UART2\r'
        msg2 = b'UART2->UART1\r'

        # Test both directions
        self.uart1.write(msg1)
        time.sleep(0.1)
        received1 = self.uart2.read(len(msg1))

        self.uart2.write(msg2)
        time.sleep(0.1)
        received2 = self.uart1.read(len(msg2))

        if received1 == msg1 and received2 == msg2:
            return TestResult.PASS, "Bidirectional loopback successful"
        else:
            return TestResult.FAIL, f"UART1->UART2: {received1 == msg1}, UART2->UART1: {received2 == msg2}"

    def test_pattern_loopback(self):
        """Test loopback with various data patterns"""
        results = []

        for pattern_name, pattern_data in self.test_patterns.items():
            if len(pattern_data) == 0:  # Skip empty pattern for loopback
                continue

            self.uart1.write(pattern_data)
            time.sleep(0.1 + len(pattern_data) * 0.0001)  # Scale timeout with data size
            received = self.uart2.read(len(pattern_data))

            success = received == pattern_data
            results.append((pattern_name, success))

            if not success:
                return TestResult.FAIL, f"Pattern '{pattern_name}' failed"

        return TestResult.PASS, f"All {len(results)} patterns passed"

    def test_large_data_loopback(self):
        """Test loopback with large data chunks"""
        sizes = [100, 500, 1000, 2000]

        for size in sizes:
            test_data = bytes([i % 256 for i in range(size)])

            self.uart1.write(test_data)
            time.sleep(0.1 + size * 0.0001)
            received = self.uart2.read(size)

            if received != test_data:
                return TestResult.FAIL, f"Large data test failed at size {size}"

        return TestResult.PASS, f"Large data test passed for sizes: {sizes}"

    # ===== COMMAND MODE TESTS =====

    def enter_command_mode(self):
        """Enter command mode on the STM32"""
        if self.command_mode:
            return True

        # Send command mode entry command
        self.uart1.write(self.CMD_ENTER_TEST_MODE)
        time.sleep(0.5)

        # Check for acknowledgment
        response = self.uart2.read(len(self.RESPONSE_READY))
        if response == self.RESPONSE_READY:
            self.command_mode = True
            return True
        return False

    def exit_command_mode(self):
        """Exit command mode and return to loopback"""
        if not self.command_mode:
            return True

        self.uart1.write(self.CMD_EXIT_TEST_MODE)
        time.sleep(0.5)

        response = self.uart2.read(len(self.RESPONSE_OK))
        if response == self.RESPONSE_OK:
            self.command_mode = False
            return True
        return False

    def test_command_mode_entry(self):
        """Test entering and exiting command mode"""
        if not self.enter_command_mode():
            return TestResult.FAIL, "Failed to enter command mode"

        if not self.exit_command_mode():
            return TestResult.FAIL, "Failed to exit command mode"

        return TestResult.PASS, "Command mode entry/exit successful"

    def test_echo_command(self):
        """Test echo command in command mode"""
        if not self.enter_command_mode():
            return TestResult.FAIL, "Failed to enter command mode"

        test_data = b'EchoTest123'
        command = self.CMD_ECHO_TEST + test_data + b'\r\n'

        self.uart1.write(command)
        time.sleep(0.2)

        # Expect: OK + echoed data
        response = self.uart2.read(len(self.RESPONSE_OK) + len(test_data))
        expected = self.RESPONSE_OK + test_data

        self.exit_command_mode()

        if response == expected:
            return TestResult.PASS, f"Echo command successful: {test_data}"
        else:
            return TestResult.FAIL, f"Expected: {expected}, Got: {response}"

    # ===== TIMING TESTS =====

    def test_transmission_timing(self):
        """Test transmission timing and throughput"""
        test_data = b'X' * 1000  # 1KB of data

        start_time = time.time()
        self.uart1.write(test_data)

        received = b''
        while len(received) < len(test_data):
            chunk = self.uart2.read(len(test_data) - len(received))
            if not chunk:
                break
            received += chunk

        end_time = time.time()
        transmission_time = end_time - start_time

        if received == test_data:
            throughput = len(test_data) / transmission_time
            return TestResult.PASS, f"Throughput: {throughput:.0f} bytes/sec"
        else:
            return TestResult.FAIL, f"Data integrity failed during timing test"

    def test_burst_transmission(self):
        """Test rapid burst transmissions"""
        burst_count = 10
        burst_data = b'BurstTest'

        # Send bursts rapidly
        for i in range(burst_count):
            self.uart1.write(burst_data)
            time.sleep(0.01)  # Small delay between bursts

        # Collect all responses
        time.sleep(0.5)
        total_expected = burst_data * burst_count
        received = self.uart2.read(len(total_expected))

        if received == total_expected:
            return TestResult.PASS, f"Burst transmission successful ({burst_count} bursts)"
        else:
            return TestResult.FAIL, f"Expected {len(total_expected)} bytes, got {len(received)}"

    # ===== ERROR CONDITION TESTS =====

    def test_buffer_overflow(self):
        """Test buffer overflow conditions"""
        # This would need to be implemented based on your buffer size
        # For now, test with a very large message
        large_data = b'X' * 5000  # 5KB

        self.uart1.write(large_data)
        time.sleep(1.0)  # Give time for processing

        # Check if any data was received (partial success acceptable)
        received = self.uart2.read(len(large_data))

        if len(received) > 0:
            return TestResult.PASS, f"Buffer test: received {len(received)}/{len(large_data)} bytes"
        else:
            return TestResult.FAIL, "No data received during buffer test"

    def test_concurrent_transmission(self):
        """Test concurrent transmission on both UARTs"""
        msg1 = b'Concurrent1'
        msg2 = b'Concurrent2'

        # Send simultaneously
        self.uart1.write(msg1)
        self.uart2.write(msg2)

        time.sleep(0.2)

        # Check both directions
        received1 = self.uart2.read(len(msg1))
        received2 = self.uart1.read(len(msg2))

        if received1 == msg1 and received2 == msg2:
            return TestResult.PASS, "Concurrent transmission successful"
        else:
            return TestResult.FAIL, f"Concurrent test failed: {received1 == msg1}, {received2 == msg2}"

    # ===== MAIN TEST EXECUTION =====

    def run_all_tests(self):
        """Run the complete test suite"""
        print("🚀 Starting UART Integration Test Suite")
        print("="*50)

        if not self.setup_connections():
            print("❌ Failed to setup UART connections")
            return 2

        try:
            # Phase 1: Loopback Tests (Smoke Tests)
            print("\n📡 Phase 1: Loopback Tests (Smoke Tests)")
            print("-" * 40)

            self.run_test_case(self.test_basic_loopback,
                              "Basic Loopback",
                              "Test basic UART loopback functionality")

            self.run_test_case(self.test_bidirectional_loopback,
                              "Bidirectional Loopback",
                              "Test loopback in both directions")

            self.run_test_case(self.test_pattern_loopback,
                              "Pattern Loopback",
                              "Test loopback with various data patterns")

            self.run_test_case(self.test_large_data_loopback,
                              "Large Data Loopback",
                              "Test loopback with large data chunks")

            # Phase 2: Command Mode Tests
            print("\n🎛️  Phase 2: Command Mode Tests")
            print("-" * 40)

            self.run_test_case(self.test_command_mode_entry,
                              "Command Mode Entry",
                              "Test entering and exiting command mode")

            self.run_test_case(self.test_echo_command,
                              "Echo Command",
                              "Test echo command in command mode")

            # Phase 3: Timing and Performance Tests
            print("\n⏱️  Phase 3: Timing and Performance Tests")
            print("-" * 40)

            self.run_test_case(self.test_transmission_timing,
                              "Transmission Timing",
                              "Test transmission timing and throughput")

            self.run_test_case(self.test_burst_transmission,
                              "Burst Transmission",
                              "Test rapid burst transmissions")

            # Phase 4: Error Condition Tests
            print("\n🔥 Phase 4: Error Condition Tests")
            print("-" * 40)

            self.run_test_case(self.test_buffer_overflow,
                              "Buffer Overflow",
                              "Test buffer overflow conditions")

            self.run_test_case(self.test_concurrent_transmission,
                              "Concurrent Transmission",
                              "Test concurrent transmission on both UARTs")

            # Results Summary
            self.print_results_summary()

            # Return appropriate exit code
            failed_tests = [t for t in self.test_results if t.result == TestResult.FAIL]
            return 1 if failed_tests else 0

        except Exception as e:
            print(f"❌ Test suite failed with exception: {e}")
            return 3
        finally:
            self.cleanup_connections()

    def print_results_summary(self):
        """Print comprehensive test results summary"""
        print("\n" + "="*60)
        print("🏁 TEST RESULTS SUMMARY")
        print("="*60)

        passed = len([t for t in self.test_results if t.result == TestResult.PASS])
        failed = len([t for t in self.test_results if t.result == TestResult.FAIL])
        skipped = len([t for t in self.test_results if t.result == TestResult.SKIP])
        total = len(self.test_results)

        print(f"Total Tests: {total}")
        print(f"✅ Passed: {passed}")
        print(f"❌ Failed: {failed}")
        print(f"⏭️  Skipped: {skipped}")

        if failed > 0:
            print(f"\n❌ FAILED TESTS:")
            for test in self.test_results:
                if test.result == TestResult.FAIL:
                    print(f"  - {test.name}: {test.details}")

        print(f"\n⏱️  Total execution time: {sum(t.execution_time for t in self.test_results):.3f}s")

        # Overall result
        if failed == 0:
            print("\n🎉 ALL TESTS PASSED!")
        else:
            print(f"\n💥 {failed} TEST(S) FAILED!")

def main():
    """Main entry point"""
    try:
        test_suite = UARTTestSuite()
        exit_code = test_suite.run_all_tests()
        print(f"\nExiting with code: {exit_code}")
        sys.exit(exit_code)

    except KeyboardInterrupt:
        print("\n⚠️  Test suite interrupted by user")
        sys.exit(130)
    except Exception as e:
        print(f"❌ Fatal error: {e}")
        sys.exit(3)

if __name__ == "__main__":
    main()
