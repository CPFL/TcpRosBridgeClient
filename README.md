# TcpRosBridgeClient Example

This simple example reads frames from the webcam and send them to the RosBridgeServer using BSON for performance.

## Library Requirements
1. OpenCV world library `opencv_world3xx.lib`
1. WinSock `ws2_lib.lib`

Modify solution to match your setup.

## How to launch

1. *On ROS Host side* Launch RosBridgeServer TCP in BSON mode
`roslaunch rosbridge_server rosbridge_tcp.launch bson_only_mode:=true`
2. *On the Windows Client side*. Execute the `RosBridgeCameraApp.exe` from a command line setting the IP and port, i.e.
`RosBridgeCameraApp.exe 172.24.1.1 9090`

The image data should be visible in the `/webcam/image` topic (`sensor_msgs/Image`).
The status will be reported in the `/webcam/image_status` topic (`std_msgs/String`)
