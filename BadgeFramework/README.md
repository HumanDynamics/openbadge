#BadgeFramework

This is a basic set of frameworks and libraries that are useful for interfacing with the badge in Python.

We currently have:

- BadgeConnection: An interface for an implementation of a badge communication layer.
- BLEBadgeConnection: An implementation of BadgeConnection that communicates with the badge over BLE using the Adafruit Bluefruit library.
- Badge: An object that communicates with a badge using the BadgeConnection
- badge_protocol: A set of BadgeMessage objects that make it easy to define, serialize, and deserialize the badge's proprietery binary communication messages.

We also include an example development terminal *terminal.py* that illustrates how one can use these various components to communicate with the badge. 

The purpose of this library is to expedite development of many tools (hub, integration test, developer terminal, etc.) that all have to communicate with the badge by forming a common, reusable codebase that all of these tools can use. As such, the framework is designed to be modular.  (E.g. the only portion of this framework that depends on a specific BLE library is BLEBadgeConnection, and one can easily swap out BadgeConnection implementations for say one that uses UART, or one can easily import the badge_protocol definitions into a different project)
