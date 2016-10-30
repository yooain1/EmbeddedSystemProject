*Source directory 구조 설명과 각 파일 역할에 대한 설명

[Source directory 구조]

Design/
	- readme.txt			: 소스 코드의 구조와 각 파일에 대한 설명

	- network/			: 네트워크 설정에 관한 디렉토리
		- interfaces_backup	: 초기 네트워크 상태 정보
		- interfaces_internet	: 인터넷 연결 네트워크 상태 정보
		- interfaces_nfs	: nfs 네트워크 상태 정보
		- set_default.sh	: 초기 네트워크 상태로 설정
		- set_internet.sh	: 인터넷 연결 네트워크 상태로 설정
		- set_nfs.sh		: nfs 네트워크 상태로 설정

	- Clock/			: 메인 프로그램 실행 코드
		- images/		: 화면 시계 출력을 위한 이미지
		- bmp_header.h		: 비트맵 출력을 위한 헤더 파일
		- Hangul_Clock.c	: 메인 프로그램 소스 코드
		- Hangul_Clock		: 메인 프로그램 실행 파일
		- time.txt		: 시간 정보 저장

	- led_ioremap/			: led 디바이스 드라이버 소스 코드
		- Makefile		: 컴파일
		- ioremap_led_dd.c	: 소스코드
		- ioremap_led_dd.ko	: 모듈 실행 파일

	- fnd_ioremap/			: fnd 디바이스 드라이버 소스 코드
		- Makefile		: 컴파일
		- ioremap_fnd_dd.c	: 소스코드
		- ioremap_fnd_dd.ko	: 모듈 실행 파일

	- push_ioremap/			: push 버튼 디바이스 드라이버 소스 코드
		- Makefile		: 컴파일
		- ioremap_push_dd.c	: 소스 코드
		- ioremap_push_dd.ko	: 모듈 실행 파일

	- kernel_timer/			: 커널 타이머 디바이스 드라이버 소스 코드
		- Makefile		: 컴파일
		- clock_timer_dd.c	: 소스 코드
		- clock_timer_dd.ko	: 모듈 실행 파일

	- compile.sh			: 전체 프로젝트 컴파일 스크립트

	- run.sh			: 전체 프로그램 실행 스크립트
