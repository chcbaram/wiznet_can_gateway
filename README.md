# wiznet_can_gateway
W5100S-EVB-Pico 보드를 이용하여 Ethernet to CAN 게이트웨이 기능을 구현하는 펌웨어 이다.

기본 개발환경 구축은 아래 링크를 참조한다.
https://blog.naver.com/chcbaram/222561278866

# Visual Studio Code 환경 변수 추가
```
		"terminal.integrated.env.windows": {
			"PATH": "${env:PATH};D:/tools/build_tools/xpack-windows-build-tools-4.2.1-2/bin;C:/fw/tools/xpack-arm-none-eabi-gcc-10.2.1-1.1/bin;D:/tools/mingw32/bin",
			"PICO_SDK_PATH": "${workspaceFolder}/external/pico-sdk",
		},
```      

# CMake Configure
```
cmake -S . -B build -G "Unix Makefiles" -DPICO_BOARD=pico
```

# CMake Build
```
cmake --build build -j4
```
