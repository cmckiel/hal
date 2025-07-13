#!/usr/bin/env python3
"""
Simple UART bridge test with proper exit codes for CI
"""
import serial
import time
import sys

def main():
    try:
        # Open serial connections
        print("Opening UART connections...")
        uart1 = serial.Serial('/dev/ttyUSB0', baudrate=115200, timeout=1)
        uart2 = serial.Serial('/dev/ttyACM0', baudrate=115200, timeout=1)

        # Wait a moment for connections to stabilize
        time.sleep(0.5)

        # Test messages
        message1 = b'Hello, is this the Krusty Krab?\r'
        message2 = b'No, this is Patrick.\r'

        print("Testing UART1 -> UART2...")
        # Send message1 from uart1 to uart2
        uart1.write(message1)
        time.sleep(0.1)
        received_message1 = uart2.read(len(message1))

        print("Testing UART2 -> UART1...")
        # Send message2 from uart2 to uart1
        uart2.write(message2)
        time.sleep(0.1)
        received_message2 = uart1.read(len(message2))

        # Check results
        test1_pass = received_message1 == message1
        test2_pass = received_message2 == message2

        print(f"UART1->UART2: {'PASS' if test1_pass else 'FAIL'}")
        print(f"  Sent:     {message1}")
        print(f"  Received: {received_message1}")

        print(f"UART2->UART1: {'PASS' if test2_pass else 'FAIL'}")
        print(f"  Sent:     {message2}")
        print(f"  Received: {received_message2}")

        # Overall result
        if test1_pass and test2_pass:
            print("✅ OVERALL: SUCCESS - All tests passed")
            exit_code = 0
        else:
            print("❌ OVERALL: FAILURE - One or more tests failed")
            exit_code = 1

    except serial.SerialException as e:
        print(f"❌ SERIAL ERROR: {e}")
        print("Check that:")
        print("  - Hardware is connected")
        print("  - Correct USB ports (/dev/ttyUSB0, /dev/ttyACM0)")
        print("  - No other processes using the ports")
        exit_code = 2

    except Exception as e:
        print(f"❌ UNEXPECTED ERROR: {e}")
        exit_code = 3

    finally:
        # Clean up - close serial ports
        try:
            if 'uart1' in locals():
                uart1.close()
            if 'uart2' in locals():
                uart2.close()
            print("Serial ports closed")
        except:
            pass

    print(f"Exiting with code: {exit_code}")
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
