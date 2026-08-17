# 휴비츠 3D 그래픽스 개발자 지원 문안

이 문서는 `DentalViz_v2_실행계획서.md`의 지원 서사를 실제 저장소 증거와 대조해 작성한
초안이다. 제출 전 본인이 실명, 연락처, 회사명, 재직 기간과 정량 성과를 최종 확인한다.

## 경력 요약

C++/MFC 기반 상용 Windows 소프트웨어 개발 경험과 GDI+, MapLibre, Fabric.js 기반의
그래픽 처리 경험을 C++20/OpenGL 3.3 기반 3D 그래픽스로 확장했습니다. DentalViz에서
STL/OBJ Mesh Rendering, Orbit/Pan/Zoom Camera, Ray Picking, 두 표면점 사이의 3D
직선거리 측정, model-space Clipping Preview를 구현했습니다. 반복적인 Material 표현은
제한된 MiniShader DSL로 정의하고, Lexer부터 GLSL 생성 및 OpenGL 후보 프로그램 검증까지
통과한 경우에만 현재 Renderer와 교체하도록 설계했습니다.

근거: `v0.5-viewer`, `v0.8-minishader` tag, 84초 Demo, 63개 Debug/Release test,
`docs/verification/`, `docs/performance/benchmark-summary.md`.

## 경력기술서에 추가할 DentalViz 항목

### DentalViz - C++ Dental 3D Visualization with MiniShader Runtime

- 기간: 2026.08
- 역할: 개인 설계·구현·테스트·배포
- 환경: C++20, OpenGL 3.3 Core, GLSL, GLFW, GLAD, GLM, Dear ImGui, Assimp, Catch2,
  CMake, vcpkg, MSVC, GitHub Actions
- Viewer: indexed VAO/VBO/EBO Mesh Rendering과 Blinn-Phong, Wireframe, Normal Color 구현
- Camera: bounds 기반 Fit을 포함한 Orbit/Pan/Zoom과 DPI 독립 Viewer 좌표 변환 구현
- Geometry: World Ray, AABB gate, Möller-Trumbore 최근접 triangle Picking과 두 표면점의
  3D Euclidean 직선거리 구현
- Interaction: model-space plane의 positive half-space를 fragment discard하는 Clipping
  Preview와 ImGui 입력 충돌 방지 구현
- MiniShader: Lexer, Parser/AST, Semantic validation, GLSL generator, 명시적 Runtime
  Compile & Apply 및 Last Known Good Shader 구현
- 품질: move-only OpenGL RAII, NaN/손상/과대/깊은 입력 방어, MSVC `/W4 /WX`,
  Debug/Release 63 tests, Windows CI, 실행 파일 외부 폴더 Smoke Test
- 성능: Release 1280×720/VSync Off 환경에서 500,004 triangle CPU frame median
  0.516 ms, Picking median 3.321 ms 측정. 두 값은 GPU 시간이 아닌 CPU 관측 시간임을 명시
- 링크: <https://github.com/leeleemaster/OpenGL_MSVC>
- Demo: <https://github.com/leeleemaster/OpenGL_MSVC/blob/main/docs/demo/DentalViz-v0.8-portfolio-demo.mp4>

## 자기소개 문안

상용 C++ 응용 소프트웨어를 개발하며 기능 구현뿐 아니라 입력 오류, 자원 수명, 배포 환경까지
함께 다뤄 왔습니다. GDI+, MapLibre, Fabric.js를 이용한 시각화 경험을 3D 그래픽스로
확장하기 위해 DentalViz를 설계했습니다. OpenGL 3.3의 indexed rendering pipeline을 직접
구성하고 Camera, Picking, 3D 직선거리 측정, Clipping Preview를 하나의 Viewer에
통합했습니다.

구현 과정에서는 화면에 보이는 결과와 내부 책임을 분리하는 데 집중했습니다. 파일에서 읽은
CPU MeshData는 기하 계산과 test에서 사용하고, VAO/VBO/EBO와 Shader Program은 move-only
RAII 객체가 소유하도록 했습니다. Ray Picking은 Viewer 좌표를 World Ray로 변환한 뒤 AABB와
triangle을 순서대로 검사하며, 측정값은 두 표면점 사이의 Euclidean 직선거리로 정의했습니다.
Clipping은 단면 생성 기능이 아니라 fragment discard 기반 Preview라고 범위를 명확히 했습니다.

MiniShader는 전체 GLSL을 재구현하려는 언어가 아닙니다. 반복 Material 표현에 필요한 작은
문법만 허용하고, Lexer, Parser, Semantic validation, GLSL generation과 OpenGL
compile/link를 통과한 후보만 현재 Renderer에 적용합니다. 오류가 발생하면 직전 정상
프로그램을 유지합니다. 63개 Debug/Release test, Windows CI, 실제 GPU 검증, 100k/500k
triangle benchmark와 독립 폴더 package 실행까지 수행했습니다. 이 프로젝트에서 확인한
3D 좌표·렌더링·Geometry Interaction의 기초와 상용 SW의 안정성 관점을 휴비츠의 Dental
3D Scanner Software 개발에 연결하고 싶습니다.

## 공고 요구 역량과 구현 근거

| 공고 항목 | DentalViz 근거 | 확인 위치 |
|---|---|---|
| C/C++ | C++20 core, RAII, move semantics, input boundary | `src/`, Commit 21 |
| OpenGL | 3.3 Core, VAO/VBO/EBO, GLSL, Camera, Picking | `src/renderer/`, Demo |
| MFC/상용 SW | 기존 경력 서사와 Windows 오류·배포 관점 연결 | 자기소개 문안, package 검증 |
| Architecture | context-free core와 OpenGL application 경계 | `docs/architecture.md` |
| CI/CD | Windows Debug/Release CMake build와 test | `.github/workflows/windows-build.yml` |
| 성능 | 100k/500k 실제 측정, raw CSV와 방법 공개 | `docs/performance/` |
| 제출 재현성 | Release ZIP, shader/license 동봉, 외부 폴더 Smoke Test | Commit 22/25 검증 |

MFC, GDI+, MapLibre, Fabric.js의 회사별 사용 기간과 실제 업무 성과는 저장소만으로 검증할 수
없으므로 본인의 기존 경력기술서와 대조한 뒤 수치를 추가한다.

## 면접 질문·답변 노트

### 왜 OpenGL 3.3 Core를 선택했나요?

VAO/VBO/EBO, Shader, uniform, framebuffer까지 modern pipeline의 핵심을 직접 구현하면서도
고급 버전 전용 기능 의존을 줄이기 위해 선택했습니다. 실제 포트폴리오 범위에서는 3.3으로
Rendering, Picking, Clipping Preview, Runtime Shader 교체를 설명할 수 있었습니다.

### 왜 VTK를 사용하지 않았나요?

제품 단계에서 VTK는 유효한 선택이지만 이 프로젝트의 목적은 Camera, 좌표 변환, Picking,
OpenGL 자원 수명을 직접 구현하고 설명하는 것이었습니다. 파일 파싱은 핵심 범위가 아니어서
Assimp를 사용했고, 렌더링과 Geometry Interaction은 직접 구현했습니다.

### Picking 성능은 어떻게 확장할 수 있나요?

현재는 AABB gate 이후 모든 triangle을 검사해 500k triangle median 3.321 ms를 측정했습니다.
모델이 커지거나 picking 빈도가 높아지면 BVH를 build하고 ray traversal 후보 triangle만
검사하는 방향이 다음 단계입니다. 현재 README에 BVH를 구현했다고 표현하지 않습니다.

### Clipping의 한계는 무엇인가요?

Fragment Shader에서 plane 한쪽을 discard하는 시각적 Preview입니다. mesh를 절단하거나
단면 cap을 만들지 않습니다. 의료용 단면 생성 기능 또는 가공 결과로 사용할 수 없습니다.

### Last Known Good은 어떻게 보장하나요?

MiniShader 결과로 별도 후보 OpenGL program을 만들고 compile/link가 성공한 경우에만
현재 program과 교체합니다. DSL semantic 오류나 driver 오류가 발생하면 후보를 폐기하고
현재 program handle을 유지합니다. 실제 GPU 검증에서도 invalid source 후 기존 화면과
program이 유지되는지 확인했습니다.

## 제출 전 본인 확인

- PDF 생성 시 `python scripts/create-portfolio-pdf.py --applicant "실명"`으로 표지 이름 확정
- 실명, 전화번호, 이메일, 회사명, 재직 기간, 프로젝트별 본인 기여도 확인
- 기존 경력의 MFC/GDI+/MapLibre/Fabric.js 사용 근거와 정량 성과 확인
- 채용 공고의 최종 직무명과 제출 채널에서 요구하는 파일 수 확인
- 과장 표현, 임상 기능 암시, 표면거리, 실시간 단면 생성 표현이 없는지 최종 검토
