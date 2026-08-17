# 커밋 26 검증: 한국어 UI와 포트폴리오

검증일: 2026-08-17

## 현지화 범위

- 뷰어의 모델 정보, 렌더링, 거리 측정, 클리핑, MiniShader 상태와 오류 문구를
  한국어로 변경했습니다.
- MiniShader의 어휘·구문·의미 분석 진단은 행/열과 코드 식별자를 유지하면서
  한국어로 표시합니다.
- `C++`, `OpenGL`, `GLSL`, `MiniShader`, API/클래스명, 코드와 파일명처럼 식별에
  필요한 항목만 영어로 남겼습니다.
- GitHub README, Windows 패키지 안내, 정적 화면 7장, 84초 시연 영상과 12페이지
  포트폴리오 PDF를 한국어 기준으로 다시 만들었습니다.

## 빌드와 실행 검증

- MSVC Debug/Release `/W4 /WX` 빌드
- Debug/Release 각각 Catch2 테스트 63/63 통과
- NVIDIA GeForce RTX 5070 Ti, OpenGL 3.3.0 NVIDIA 591.86에서 실제 실행
- MiniShader 정상 적용 2회 확인
- 잘못된 소스 적용 후 마지막 정상 셰이더의 화면 해시 유지 확인

MiniShader 기본 화면 SHA-256은
`313273ECD8944ABBA72242B42428BD69E685F63135F8B07B9D0733C2A551970F`입니다.
수정 적용 화면과 오류 이후 화면의 SHA-256은 모두
`F7DD0A6E3E5370BFB236B60DD8CF8DE15E2337689C1B73B43B4B66113AFEF89A`로
일치했습니다.

## 포트폴리오 검증

- 84초 영상은 12 FPS, 1,008프레임으로 생성했고 정상 적용 2회와 오류 1회를
  자동 검증했습니다.
- 주요 시점 9개 프레임에서 한글 자막, 클리핑, MiniShader 적용·오류 상태를
  직접 확인했습니다.
- PDF는 A4 가로 12페이지이며 전 페이지를 이미지로 렌더링해 겹침과 잘림을
  확인했습니다.
- PDF의 링크 주석은 6개이며, 마지막 페이지의 QR 2개는 저장소와 시연 영상 URL로
  정상 해독됐습니다.

## Windows 패키지와 체크섬

`DentalViz-v1.0.1-korean-windows-x64.zip`을 새 Windows 임시 폴더에 풀어
`DentalViz 1.0.1` 실행과 5초 후 정상 종료를 확인했습니다. 임시 폴더는 검증 후
삭제했습니다.

| 자산 | SHA-256 |
|---|---|
| `DentalViz-v1.0.1-korean-windows-x64.zip` | `0A2070C6BE64C6EC637BB0BF18C4396EBAB6F0515364E61D5E493BF8FBD7F738` |
| `DentalViz_Huvitz_Portfolio.pdf` | `878F2B522880655F3EAC1B2624DA88753AC870E083E625C3CB22F54A82A34EF7` |
| `DentalViz-v0.8-portfolio-demo.mp4` | `9284EC80DBA66D1C4DEA16C7B151798BDA4234C610E1B3578B75B221590D9D74` |

`output/release/SHA256SUMS.txt`의 세 항목을 실제 파일과 다시 대조해 모두 통과했습니다.
