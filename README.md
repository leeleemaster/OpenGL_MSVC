# DentalViz

C++20과 OpenGL을 이용해 치과용 삼각형 메시를 탐색하고 측정하는 Windows 데스크톱 뷰어입니다.

현재 단계는 외부 GLSL과 복수 렌더 모드를 사용해 STL/OBJ 메시를 불러오고 탐색하는 단계입니다.

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

Debug 실행 파일은 다음과 같이 실행합니다. 왼쪽 드래그는 Orbit, 가운데 드래그는 Pan,
휠은 Zoom, `F`는 모델 맞춤입니다. 숫자 `1`, `2`, `3`으로 각각 Solid, Wireframe,
Normal Color 모드를 선택하며 `Escape` 또는 닫기 버튼으로 종료합니다.

```powershell
./out/build/msvc/Debug/DentalViz.exe
```

외부 STL 또는 OBJ를 시작 시 로드할 수 있습니다. STL에는 단위 메타데이터가 없으므로
현재 표시는 `1 model unit = 1 mm`라는 명시적인 가정을 사용합니다.

```powershell
./out/build/msvc/Debug/DentalViz.exe --model "C:\Models\dental.stl"
```

파일이 없거나 손상된 경우 오류를 출력하고 절차 생성 테스트 치아로 복구합니다.

### VS Code

Microsoft `C/C++` 및 `CMake Tools` 확장이 설치된 VS Code에서 저장소 폴더를 신뢰한 뒤
`F5`를 누르면 Debug 빌드, 테스트, 실행이 순서대로 진행됩니다. `Ctrl+Shift+B`는 기본
Debug 빌드를 실행하며, `Tasks: Run Task`에서 Release 빌드도 선택할 수 있습니다.

## 현재 상태

- [x] 범위, 좌표계, 완료조건 고정
- [x] CMake/MSVC/vcpkg 골격
- [x] Catch2 Smoke Test
- [x] Windows GitHub Actions 구성
- [x] OpenGL Window와 Application Loop
- [x] Indexed GPU Mesh와 Blinn–Phong 테스트 렌더링
- [x] 절차 생성 치아의 법선·경계·인덱스 단위 테스트
- [x] Orbit/Pan/Zoom과 Bounds 기반 `F` 모델 맞춤 카메라
- [x] 외부 GLSL 로딩과 파일·단계별 컴파일 오류 표시
- [x] Blinn–Phong Solid/Wireframe/Normal Color 렌더 모드
- [x] Assimp 기반 STL/OBJ 로딩, 법선·인덱스·노드 변환 전처리
- [ ] 출처와 재배포 라이선스가 확인된 Dental STL 확보
- [ ] Dear ImGui Viewer UI

자세한 범위와 기술 결정은 [`docs/scope.md`](docs/scope.md),
[`docs/coordinate-system.md`](docs/coordinate-system.md),
[`docs/architecture.md`](docs/architecture.md)를 참고하세요. 실제 OpenGL 로컬 실행 결과는
[`docs/verification/`](docs/verification/)에 단계별로 기록되어 있습니다.

## 라이선스

소스 코드는 MIT License로 배포합니다. 모델과 제3자 라이브러리는 각각의 라이선스를 따릅니다.
