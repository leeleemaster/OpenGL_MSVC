# DentalViz v1.0

C++20/OpenGL 3.3 Core 기반 Dental 3D Viewer와 제한된 MiniShader Runtime의 제출용
릴리스입니다.

## 포함 자산

- `DentalViz-v1.0-submission-windows-x64.zip`: Windows x64 실행 패키지
- `DentalViz_Huvitz_Portfolio.pdf`: 12페이지 기술 포트폴리오
- `DentalViz-v0.8-portfolio-demo.mp4`: 실제 Release UI 84초 Demo
- `SHA256SUMS.txt`: 위 세 자산의 SHA-256

## 검증

- MSVC Release `/W4 /WX`
- Debug/Release 63개 Catch2 test
- NVIDIA GeForce RTX 5070 Ti/OpenGL 3.3 실제 GPU Runtime 검증
- 새 임시 폴더에서 ZIP 해제 후 `DentalViz 1.0.0` 실행과 정상 종료
- MiniShader 정상 적용 2회와 오류 시 Last Known Good 유지

## 범위

거리 측정은 두 표면점 사이의 3D 직선거리입니다. Clipping은 fragment discard 기반
Preview이며 단면 mesh/cap을 만들지 않습니다. 프로젝트는 비임상 포트폴리오 prototype이며
의료기기 또는 진단 도구가 아닙니다.
