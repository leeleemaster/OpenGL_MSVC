# DentalViz v1.0.1 한국어판

C++20/OpenGL 3.3 Core 기반 치과 3D 뷰어와 제한된 MiniShader 실행 환경의
한국어 포트폴리오 릴리스입니다.

## 포함 자산

- `DentalViz-v1.0.1-korean-windows-x64.zip`: Windows x64 한국어 실행 패키지
- `DentalViz_Huvitz_Portfolio.pdf`: 한국어 중심 12페이지 기술 포트폴리오
- `DentalViz-v0.8-portfolio-demo.mp4`: 실제 한국어 Release UI 84초 시연
- `SHA256SUMS.txt`: 위 세 자산의 SHA-256

## 검증

- MSVC Release `/W4 /WX`
- Debug/Release 63개 Catch2 테스트
- NVIDIA GeForce RTX 5070 Ti/OpenGL 3.3 실제 GPU 실행 검증
- 새 임시 폴더에서 ZIP 해제 후 `DentalViz 1.0.1` 실행과 정상 종료
- MiniShader 정상 적용 2회와 오류 시 마지막 정상 셰이더 유지

## 범위

거리 측정은 두 표면점 사이의 3D 직선거리입니다. 클리핑은 프래그먼트 폐기 기반
미리보기이며 단면 메시나 덮개를 만들지 않습니다. 프로젝트는 비임상 포트폴리오
시제품이며 의료기기 또는 진단 도구가 아닙니다.
