
# pwr-probe-test

This is a project of a hardware tester of an EV charger for servicing purposes.
This includes FPGA, RCD, RELAYS, VOLTAGES, eMETERS, LOCK MOTOR and Comms tests.
Main chip: PIC24EP256MC206

Dev state: Initialization

## Structure

| Path                              | Purpose                                                                                                                             |
|-----------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| _build                            | The [CMake build tree](https://cmake.org/cmake/help/latest/manual/cmake.1.html#introduction-to-cmake-buildsystems), can be deleted. |
| cmake                             | Generated [CMake](https://cmake.org/) files. May be deleted if user.cmake has not been added                                        |
| .vscode                           | See [VSCode](https://code.visualstudio.com/docs/getstarted/settings)                                                                |
| .vscode\settings.json             | Workspace specific settings                                                                                                         |
| .vscode\pwr-probe-test.mplab.json | The MPLAB project file, should not be deleted                                                                                       |
| out                               | Final build artifacts                                                                                                               |
