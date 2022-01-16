# Ethernet CAN Gateway
W5100S-EVB-Pico 보드를 이용하여 Ethernet to CAN 게이트웨이 기능을 구현하는 펌웨어 이다.

## 개발환경 
기본 개발환경 구축은 아래 링크를 참조한다.
- https://blog.naver.com/chcbaram/222561278866

## Visual Studio Code 환경 변수 추가
- folder path에 맞게 수정하여 추가 (workspace에 추가를 추천)
```
"terminal.integrated.env.windows": {
	"PATH": "${env:PATH};D:/tools/build_tools/xpack-windows-build-tools-4.2.1-2/bin;C:/fw/tools/xpack-arm-none-eabi-gcc-10.2.1-1.1/bin;D:/tools/mingw32/bin",
},
```

## CMake Configure
```
cmake -S . -B build -G "Unix Makefiles" -DPICO_BOARD=pico
```

## CMake Build
```
cmake --build build -j4
```

## Optional
### [Task Runner Extention](https://marketplace.visualstudio.com/items?itemName=SanaAjani.taskrunnercode)
- VS Code extension to view and run tasks from Explorer pane
- 아래와 같이 Build와 Download를 위한 run task를 등록하여 쉽게 사용 가능
	```
	// For tasks.json
	{
		// See https://go.microsoft.com/fwlink/?LinkId=733558
		// for the documentation about the tasks.json format
		"version": "2.0.0",
		"tasks": [
			{
				"label": "cmake_build",
				"type": "shell",
				"command": "cmake --build build -j4",
				"problemMatcher": [],
				"group": {
					"kind": "build",
					"isDefault": true
				}
			},
			{
				"label": "uf2_down",
				"type": "shell",
				"command": "python ./tools/down.py COM50",
			}
		]
	} 
	```
