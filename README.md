# DentalViz

C++20과 OpenGL을 이용해 치과용 삼각형 메시를 탐색하고 측정하는 Windows 데스크톱 뷰어입니다.

현재 단계는 재현 가능한 MSVC 빌드 환경 위에 OpenGL 애플리케이션 루프를 구성하는 단계입니다.

## 목표 기능

- OpenGL 3.3 Core 기반 메시 렌더링
- Orbit/Pan/Zoom 카메라와 모델 맞춤 보기
- STL 메시 로딩과 모델 정보 표시
- Ray–Triangle 기반 Picking
- 두 표면점 사이의 3차원 직선거리 측정
- Fragment discard 기반 Clipping Preview
- 선택 기능: MiniShader Runtime Compile & Apply

## 로컬 빌드

Visual Studio 2022에서 **Desktop development with C++** 워크로드가 설치되어 있어야 합니다.
일반 PowerShell에서 다음 스크립트를 실행하면 Visual Studio에 포함된 CMake와 vcpkg를 자동으로 찾습니다.

```powershell
./scripts/build.ps1 -Configuration Debug
./scripts/build.ps1 -Configuration Release
```

빌드 결과는 `out/build/msvc/`에 생성됩니다. 테스트는 빌드 후 자동으로 실행됩니다.

Debug 실행 파일은 다음과 같이 실행합니다. 창은 `Escape` 키 또는 닫기 버튼으로 종료할 수 있습니다.

```powershell
./out/build/msvc/Debug/DentalViz.exe
```

## 현재 상태

- [x] 범위, 좌표계, 완료조건 고정
- [x] CMake/MSVC/vcpkg 골격
- [x] Catch2 Smoke Test
- [x] Windows GitHub Actions 구성
- [x] OpenGL Window와 Application Loop

자세한 범위와 기술 결정은 [`docs/scope.md`](docs/scope.md),
[`docs/coordinate-system.md`](docs/coordinate-system.md),
[`docs/architecture.md`](docs/architecture.md)를 참고하세요. 실제 OpenGL 로컬 실행 결과는
[`docs/verification/commit-02.md`](docs/verification/commit-02.md)에 기록되어 있습니다.

## 라이선스

소스 코드는 MIT License로 배포합니다. 모델과 제3자 라이브러리는 각각의 라이선스를 따릅니다.
