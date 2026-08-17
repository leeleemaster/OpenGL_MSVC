# DentalViz v1.0 제출 체크리스트

기술 산출물은 2026-08-17 기준으로 완료했다. 아래의 지원자 확인 항목은 저장소에서 사실을
확인할 수 없거나 실제 채용 사이트 조작이 필요한 항목이므로 제출 직전에 본인이 완료한다.

## 기술 산출물 - 완료

- [x] CMake/MSVC Release `/W4 /WX` 빌드
- [x] Debug/Release Catch2 테스트 63개
- [x] 실제 RTX 5070 Ti/OpenGL 3.3 실행 안정성 검증
- [x] MiniShader 정상 적용 2회와 오류 시 마지막 정상 셰이더 유지 검증
- [x] Windows x64 ZIP에 실행 파일, 셰이더 6개, 제3자 고지 17건 포함
- [x] ZIP을 새 임시 폴더에 해제하고 빌드 폴더 밖에서 `DentalViz 1.0.1` 실행
- [x] GitHub 저장소, 84초 시연, 12페이지 포트폴리오 PDF 공개 링크 확인
- [x] `v1.0-submission` 소스 태그와 기존 GitHub 릴리스
- [ ] `v1.0.1-korean` 소스 태그와 한국어판 GitHub 릴리스
- [x] ZIP/PDF/MP4 SHA-256 기록

## 지원자 확인 - 제출 전 필요

- [ ] PDF 표지의 `leeleemaster`를 지원서와 동일한 실명으로 생성할지 결정
- [ ] 전화번호, 이메일, 회사명, 재직 기간, 프로젝트별 본인 기여도 대조
- [ ] MFC/GDI+/MapLibre/Fabric.js 경력의 회사별 사용 기간과 정량 성과 확인
- [ ] 휴비츠 채용 공고의 최종 직무명, 마감일, 첨부 파일 수 재확인
- [ ] 리멤버 입력 내용과 경력기술서·포트폴리오 문구 대조
- [ ] 최종 파일 업로드 후 권한/다운로드 확인
- [ ] 실제 지원 제출과 완료 화면 보관

실명 PDF 생성 명령:

```powershell
python ./scripts/create-portfolio-pdf.py --applicant "지원자 실명"
```

공개 저장소에는 개인정보를 추가하기 전에 공개 범위를 반드시 확인한다.
