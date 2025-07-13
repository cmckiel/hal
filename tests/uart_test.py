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
        long_test_message = b'The quick brown fox jumps over the lazy dog. This pangram contains every letter of the English alphabet at least once, making it an excellent test for character encoding and transmission integrity. In telecommunications and data transmission testing, comprehensive messages like this help verify that all ASCII characters can be properly sent, received, and echoed back without corruption or loss. Additional test patterns include numbers: 0123456789, special characters: !@#$%^&*()_+-=[]{}|;:,.<>?, and various punctuation marks. Extended ASCII testing requires characters like: ÄÖÜäöüß€£¥©®™. For binary data simulation, we include escape sequences: \\r\\n\\t\\0, and control characters that might cause parsing issues in poorly implemented drivers. Network protocols often use structured data formats, so JSON-like syntax testing is valuable: {"key": "value", "array": [1, 2, 3], "nested": {"boolean": true, "null": null}}. XML-style markup is also common: <root><element attribute="value">content</element></root>. Database queries and SQL injection testing might include: SELECT * FROM users WHERE id = 1; DROP TABLE users; -- Modern applications frequently handle Unicode characters: 你好世界, مرحبا بالعيد, Здравствуй мир, こんにちは世界. Hexadecimal patterns for low-level testing: 0x00, 0xFF, 0xAA, 0x55, 0xDEADBEEF. Base64 encoded data: SGVsbG8gV29ybGQh. URL encoded strings: Hello%20World%21. This comprehensive test message should thoroughly exercise UART drivers, interrupt handlers, buffer management, and data integrity mechanisms across various scenarios and edge cases that might occur in real-world embedded systems applications.\\r'
        message1 = b'Hello, is this the Krusty Krab?\r'
        message2 = b'No, this is Patrick.\r'

        print("Testing UART1 Loopback with short message...")
        # Send message1 to uart1
        uart1.write(message1)
        time.sleep(0.1)
        received_message1 = uart1.read(len(message1))

        print("Testing UART2 Loopback with short message...")
        # Send message2 to uart2
        uart2.write(message2)
        time.sleep(0.1)
        received_message2 = uart2.read(len(message2))

        print("Testing UART1 Loopback with long message...")
        # Send message1 to uart1
        uart1.write(long_test_message)
        time.sleep(0.1)
        received_long_message1 = uart1.read(len(long_test_message))

        print("Testing UART2 Loopback with long message...")
        # Send message1 to uart2
        uart2.write(long_test_message)
        time.sleep(0.1)
        received_long_message2 = uart2.read(len(long_test_message))

        # Check results
        test1_pass = received_message1 == message1
        test2_pass = received_message2 == message2
        test3_pass = received_long_message1 == long_test_message
        test4_pass = received_long_message2 == long_test_message

        print(f"UART1 Loopback: {'PASS' if test1_pass else 'FAIL'}")
        print(f"  Sent:     {message1}")
        print(f"  Received: {received_message1}")

        print(f"UART2 Loopback: {'PASS' if test2_pass else 'FAIL'}")
        print(f"  Sent:     {message2}")
        print(f"  Received: {received_message2}")

        print(f"UART1 Long Loopback: {'PASS' if test3_pass else 'FAIL'}")
        print(f"  Sent:     {long_test_message}")
        print(f"  Received: {received_long_message1}")

        print(f"UART2 Long Loopback: {'PASS' if test4_pass else 'FAIL'}")
        print(f"  Sent:     {long_test_message}")
        print(f"  Received: {received_long_message2}")

        # Overall result
        if test1_pass and test2_pass and test3_pass and test4_pass:
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
