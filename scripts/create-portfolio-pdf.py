from __future__ import annotations

import argparse
from io import BytesIO
from pathlib import Path

import qrcode
from PIL import Image
from reportlab.lib.colors import Color, HexColor, white
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas


PAGE_WIDTH, PAGE_HEIGHT = landscape(A4)
MARGIN = 42

INK = HexColor("#102A33")
MUTED = HexColor("#58717A")
TEAL = HexColor("#0A7D87")
CYAN = HexColor("#24B8C4")
PALE = HexColor("#E7F3F4")
PAPER = HexColor("#F5F8F8")
GOLD = HexColor("#D7A94C")
RED = HexColor("#B94B52")
CHARCOAL = HexColor("#0B2027")

REPOSITORY_URL = "https://github.com/leeleemaster/OpenGL_MSVC"
DEMO_URL = (
    "https://github.com/leeleemaster/OpenGL_MSVC/blob/main/"
    "docs/demo/DentalViz-v0.8-portfolio-demo.mp4"
)
CI_URL = REPOSITORY_URL + "/actions/workflows/windows-build.yml"


def register_fonts() -> None:
    fonts = Path("C:/Windows/Fonts")
    pdfmetrics.registerFont(TTFont("Malgun", str(fonts / "malgun.ttf")))
    pdfmetrics.registerFont(TTFont("MalgunBold", str(fonts / "malgunbd.ttf")))
    pdfmetrics.registerFont(TTFont("Consolas", str(fonts / "consola.ttf")))
    pdfmetrics.registerFont(TTFont("ConsolasBold", str(fonts / "consolab.ttf")))


def wrap_text(text: str, font: str, size: float, max_width: float) -> list[str]:
    words: list[str] = []
    for word in text.split():
        if pdfmetrics.stringWidth(word, font, size) <= max_width:
            words.append(word)
            continue
        chunk = ""
        for character in word:
            candidate = chunk + character
            if chunk and pdfmetrics.stringWidth(candidate, font, size) > max_width:
                words.append(chunk)
                chunk = character
            else:
                chunk = candidate
        if chunk:
            words.append(chunk)
    if not words:
        return [""]
    lines: list[str] = []
    current = words[0]
    for word in words[1:]:
        candidate = current + " " + word
        if pdfmetrics.stringWidth(candidate, font, size) <= max_width:
            current = candidate
        else:
            lines.append(current)
            current = word
    lines.append(current)
    return lines


def draw_paragraph(
    pdf: canvas.Canvas,
    text: str,
    x: float,
    y: float,
    width: float,
    *,
    size: float = 11,
    leading: float = 17,
    color: Color = INK,
    font: str = "Malgun",
) -> float:
    pdf.setFont(font, size)
    pdf.setFillColor(color)
    for line in wrap_text(text, font, size, width):
        pdf.drawString(x, y, line)
        y -= leading
    return y


def draw_bullets(
    pdf: canvas.Canvas,
    items: list[str],
    x: float,
    y: float,
    width: float,
    *,
    size: float = 10.5,
    leading: float = 16,
    color: Color = INK,
) -> float:
    for item in items:
        lines = wrap_text(item, "Malgun", size, width - 18)
        pdf.setFillColor(TEAL)
        pdf.circle(x + 4, y + 3, 2.2, fill=1, stroke=0)
        pdf.setFillColor(color)
        pdf.setFont("Malgun", size)
        for index, line in enumerate(lines):
            pdf.drawString(x + 16, y, line)
            y -= leading
            if index == 0:
                continue
        y -= 4
    return y


def draw_footer(pdf: canvas.Canvas, page_number: int) -> None:
    pdf.setStrokeColor(HexColor("#C8D7DA"))
    pdf.line(MARGIN, 25, PAGE_WIDTH - MARGIN, 25)
    pdf.setFillColor(MUTED)
    pdf.setFont("Malgun", 7.5)
    pdf.drawString(MARGIN, 12, "DentalViz · C++20 / OpenGL 3.3 Core · leeleemaster")
    pdf.drawRightString(PAGE_WIDTH - MARGIN, 12, f"{page_number:02d} / 12")


def begin_page(
    pdf: canvas.Canvas,
    page_number: int,
    kicker: str,
    title: str,
    subtitle: str = "",
) -> None:
    pdf.setFillColor(PAPER)
    pdf.rect(0, 0, PAGE_WIDTH, PAGE_HEIGHT, fill=1, stroke=0)
    pdf.setFillColor(TEAL)
    pdf.roundRect(MARGIN, PAGE_HEIGHT - 51, 106, 19, 9, fill=1, stroke=0)
    pdf.setFillColor(white)
    pdf.setFont("MalgunBold", 8)
    pdf.drawCentredString(MARGIN + 53, PAGE_HEIGHT - 44, kicker.upper())
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 24)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 83, title)
    if subtitle:
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 9)
        pdf.drawRightString(PAGE_WIDTH - MARGIN, PAGE_HEIGHT - 76, subtitle)
    draw_footer(pdf, page_number)


def draw_card(
    pdf: canvas.Canvas,
    x: float,
    y: float,
    width: float,
    height: float,
    *,
    fill: Color = white,
    stroke: Color = HexColor("#D6E3E5"),
    radius: float = 10,
) -> None:
    pdf.setFillColor(fill)
    pdf.setStrokeColor(stroke)
    pdf.setLineWidth(0.8)
    pdf.roundRect(x, y, width, height, radius, fill=1, stroke=1)


def draw_image_fit(
    pdf: canvas.Canvas,
    path: Path,
    x: float,
    y: float,
    width: float,
    height: float,
    *,
    background: Color = CHARCOAL,
) -> None:
    draw_card(pdf, x, y, width, height, fill=background, stroke=background, radius=8)
    with Image.open(path) as image:
        image_width, image_height = image.size
    scale = min((width - 8) / image_width, (height - 8) / image_height)
    rendered_width = image_width * scale
    rendered_height = image_height * scale
    pdf.drawImage(
        str(path),
        x + (width - rendered_width) / 2,
        y + (height - rendered_height) / 2,
        rendered_width,
        rendered_height,
        preserveAspectRatio=True,
        mask="auto",
    )


def draw_label(pdf: canvas.Canvas, text: str, x: float, y: float, color: Color = TEAL) -> None:
    width = pdfmetrics.stringWidth(text, "MalgunBold", 8) + 18
    pdf.setFillColor(color)
    pdf.roundRect(x, y - 3, width, 17, 8, fill=1, stroke=0)
    pdf.setFillColor(white)
    pdf.setFont("MalgunBold", 8)
    pdf.drawString(x + 9, y + 2, text)


def draw_metric(
    pdf: canvas.Canvas,
    x: float,
    y: float,
    width: float,
    value: str,
    label: str,
    detail: str = "",
) -> None:
    draw_card(pdf, x, y, width, 72, fill=white)
    pdf.setFillColor(TEAL)
    pdf.setFont("MalgunBold", 20)
    pdf.drawString(x + 16, y + 39, value)
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 9)
    pdf.drawString(x + 16, y + 22, label)
    if detail:
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 7.2)
        pdf.drawString(x + 16, y + 9, detail)


def qr_image(url: str) -> ImageReader:
    image = qrcode.make(url, border=2)
    buffer = BytesIO()
    image.save(buffer, format="PNG")
    buffer.seek(0)
    return ImageReader(buffer)


def draw_qr_link(
    pdf: canvas.Canvas,
    url: str,
    label: str,
    x: float,
    y: float,
    size: float = 82,
) -> None:
    pdf.setFillColor(white)
    pdf.roundRect(x - 7, y - 24, size + 14, size + 36, 9, fill=1, stroke=0)
    pdf.drawImage(qr_image(url), x, y, size, size, mask="auto")
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 8)
    pdf.drawCentredString(x + size / 2, y - 13, label)
    pdf.linkURL(url, (x - 7, y - 24, x + size + 7, y + size + 12), relative=0)


def draw_link(
    pdf: canvas.Canvas,
    label: str,
    url: str,
    x: float,
    y: float,
    width: float,
    display: str | None = None,
) -> None:
    pdf.setFillColor(TEAL)
    pdf.setFont("MalgunBold", 9)
    pdf.drawString(x, y, label)
    pdf.setFillColor(MUTED)
    pdf.setFont("Consolas", 7.1)
    for line in wrap_text(display or url, "Consolas", 7.1, width):
        y -= 12
        pdf.drawString(x, y, line)
    pdf.linkURL(url, (x, y - 3, x + width, y + 27), relative=0)


def draw_pipeline(
    pdf: canvas.Canvas,
    nodes: list[tuple[str, str]],
    x: float,
    y: float,
    total_width: float,
    *,
    height: float = 58,
) -> None:
    gap = 16
    node_width = (total_width - gap * (len(nodes) - 1)) / len(nodes)
    for index, (title, detail) in enumerate(nodes):
        node_x = x + index * (node_width + gap)
        draw_card(pdf, node_x, y, node_width, height, fill=white)
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 9)
        pdf.drawCentredString(node_x + node_width / 2, y + 34, title)
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 6.7)
        pdf.drawCentredString(node_x + node_width / 2, y + 18, detail)
        if index < len(nodes) - 1:
            start = node_x + node_width + 3
            pdf.setStrokeColor(CYAN)
            pdf.setLineWidth(2)
            pdf.line(start, y + height / 2, start + gap - 6, y + height / 2)
            pdf.setFillColor(CYAN)
            pdf.line(start + gap - 10, y + height / 2 + 3, start + gap - 6, y + height / 2)
            pdf.line(start + gap - 10, y + height / 2 - 3, start + gap - 6, y + height / 2)


def page_cover(pdf: canvas.Canvas, screenshots: Path, applicant: str) -> None:
    pdf.setFillColor(CHARCOAL)
    pdf.rect(0, 0, PAGE_WIDTH, PAGE_HEIGHT, fill=1, stroke=0)
    pdf.setFillColor(Color(0.05, 0.28, 0.32, alpha=1))
    pdf.circle(PAGE_WIDTH - 75, PAGE_HEIGHT - 70, 170, fill=1, stroke=0)
    pdf.setFillColor(Color(0.08, 0.48, 0.52, alpha=1))
    pdf.circle(PAGE_WIDTH - 5, -15, 220, fill=1, stroke=0)
    draw_image_fit(pdf, screenshots / "01_overview.png", 405, 105, 382, 310)
    pdf.setFillColor(CYAN)
    pdf.setFont("MalgunBold", 10)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 66, "휴비츠 3D 그래픽스 개발자 포트폴리오")
    pdf.setFillColor(white)
    pdf.setFont("MalgunBold", 38)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 134, "DentalViz")
    pdf.setFont("MalgunBold", 19)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 171, "C++ 기반 치과 3D 시각화")
    pdf.drawString(MARGIN, PAGE_HEIGHT - 198, "MiniShader 실행 환경")
    pdf.setFillColor(HexColor("#A7CED1"))
    pdf.setFont("Malgun", 10)
    draw_paragraph(
        pdf,
        "상용 C++ 응용 소프트웨어 경험을 OpenGL 기반 3D 뷰어와 기하 상호작용, "
        "안전한 실행 중 셰이더 교체 구조로 확장한 실구현 프로젝트",
        MARGIN,
        PAGE_HEIGHT - 235,
        315,
        size=10,
        leading=16,
        color=HexColor("#C7E0E2"),
    )
    pdf.setFillColor(GOLD)
    pdf.roundRect(MARGIN, 104, 248, 48, 8, fill=1, stroke=0)
    pdf.setFillColor(CHARCOAL)
    pdf.setFont("MalgunBold", 11)
    pdf.drawString(MARGIN + 15, 133, applicant)
    pdf.setFont("Malgun", 8.5)
    pdf.drawString(MARGIN + 15, 118, "휴비츠 3D 그래픽스 개발자 지원 · 2026.08")
    draw_qr_link(pdf, REPOSITORY_URL, "GitHub", 300, 67, 76)
    pdf.setFillColor(HexColor("#A7CED1"))
    pdf.setFont("Malgun", 7.5)
    pdf.drawString(MARGIN, 34, "비임상 포트폴리오 시제품 · 모델 단위 · 3D 직선거리")
    pdf.showPage()


def page_background(pdf: canvas.Canvas) -> None:
    begin_page(pdf, 2, "01 · 배경", "배경과 목표", "상용 응용 소프트웨어에서 치과 3D 그래픽스로")
    draw_card(pdf, MARGIN, 82, 345, 385, fill=white)
    draw_label(pdf, "경력 서사", MARGIN + 20, 435)
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 18)
    pdf.drawString(MARGIN + 20, 395, "그래픽 처리 경험을")
    pdf.drawString(MARGIN + 20, 369, "3D 파이프라인으로 확장")
    draw_paragraph(
        pdf,
        "C++/MFC 기반 상용 소프트웨어 개발과 GDI+, MapLibre, Fabric.js 기반의 "
        "시각화 경험을 바탕으로, 렌더링부터 기하 상호작용까지 직접 검증할 수 "
        "있는 OpenGL 프로젝트를 설계했습니다.",
        MARGIN + 20,
        330,
        302,
        size=10.5,
        leading=18,
    )
    draw_bullets(
        pdf,
        [
            "문제: 치과 메시를 탐색하고 표면 위치와 직선거리를 확인하는 Windows 도구",
            "목표: 카메라, 렌더링, 피킹, 거리 측정, 클리핑을 하나의 뷰어에 통합",
            "확장: 반복 재질 표현을 제한된 MiniShader DSL과 명시적 적용 흐름으로 연결",
        ],
        MARGIN + 20,
        235,
        302,
    )
    right_x = 420
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 15)
    pdf.drawString(right_x, 438, "설계 기준")
    standards = [
        ("실행 가능", "MSVC 릴리스와 Windows ZIP을 실제 GPU에서 검증"),
        ("경계 명확", "직선거리, 클리핑 미리보기, 비임상 시제품으로 정확히 표현"),
        ("근거 중심", "테스트 63개, CI, 벤치마크, 시연을 기술 문장과 연결"),
        ("안전한 교체", "오류가 난 후보 셰이더는 현재 프로그램을 바꾸지 않음"),
    ]
    y = 375
    for index, (title, detail) in enumerate(standards, start=1):
        draw_card(pdf, right_x, y, 366, 65, fill=PALE)
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 15)
        pdf.drawString(right_x + 15, y + 34, f"0{index}")
        pdf.setFillColor(INK)
        pdf.setFont("MalgunBold", 10)
        pdf.drawString(right_x + 53, y + 39, title)
        draw_paragraph(pdf, detail, right_x + 53, y + 22, 292, size=8, leading=12, color=MUTED)
        y -= 77
    pdf.showPage()


def page_features(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 3, "02 · 제품", "기능 개요", "실제 릴리스 UI · 1280×720")
    draw_image_fit(pdf, screenshots / "01_overview.png", MARGIN, 193, 475, 274)
    features = [
        ("01", "회전 / 이동 / 확대", "경계 기반 화면 맞춤 포함"),
        ("02", "메시 렌더링", "솔리드 / 와이어프레임 / 법선"),
        ("03", "광선 피킹", "최근접 삼각형 선택"),
        ("04", "3D 거리 측정", "두 표면점의 직선거리"),
        ("05", "클리핑 미리보기", "모델 좌표 Plane의 Fragment 폐기"),
        ("06", "MiniShader", "검증 후 실행 중 적용"),
    ]
    start_x, start_y = 545, 392
    for index, (number, title, detail) in enumerate(features):
        column = index % 2
        row = index // 2
        x = start_x + column * 124
        y = start_y - row * 91
        draw_card(pdf, x, y, 112, 78, fill=white)
        pdf.setFillColor(CYAN)
        pdf.setFont("MalgunBold", 8)
        pdf.drawString(x + 12, y + 55, number)
        pdf.setFillColor(INK)
        pdf.setFont("MalgunBold", 8.5)
        pdf.drawString(x + 12, y + 37, title)
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 6.6)
        for line_index, line in enumerate(wrap_text(detail, "Malgun", 6.6, 90)):
            pdf.drawString(x + 12, y + 21 - line_index * 10, line)
    draw_card(pdf, MARGIN, 76, 745, 87, fill=CHARCOAL, stroke=CHARCOAL)
    pdf.setFillColor(CYAN)
    pdf.setFont("MalgunBold", 9)
    pdf.drawString(MARGIN + 18, 137, "직접 구현 범위")
    pdf.setFillColor(white)
    pdf.setFont("Malgun", 8.5)
    pdf.drawString(MARGIN + 18, 116, "렌더러 · 카메라 · 경계 · 피킹 · 거리 측정 · 클리핑 · MiniShader 컴파일러/실행 환경")
    pdf.setFillColor(HexColor("#9FC5C8"))
    pdf.setFont("Malgun", 7.4)
    pdf.drawString(MARGIN + 18, 94, "사용 라이브러리: GLFW / GLAD / GLM / Dear ImGui / Assimp / Catch2 · 역할과 소유권 경계를 분리")
    pdf.showPage()


def page_architecture(pdf: canvas.Canvas) -> None:
    begin_page(pdf, 4, "03 · 아키텍처", "테스트 가능한 핵심부와 OpenGL 경계", "CPU 데이터와 GPU 핸들의 소유권 분리")
    left_x, right_x = MARGIN, 445
    draw_card(pdf, left_x, 128, 360, 330, fill=white)
    draw_label(pdf, "OPENGL 비의존 핵심부", left_x + 18, 426)
    core_nodes = [
        ("메시 입출력", "Assimp → 표준 MeshData"),
        ("기하 연산", "경계 · 광선 · AABB · 삼각형"),
        ("카메라", "View / Projection · DPI 변환"),
        ("MiniShader", "어휘 분석 → AST → 의미 분석 → GLSL"),
    ]
    y = 350
    for title, detail in core_nodes:
        draw_card(pdf, left_x + 20, y, 320, 51, fill=PALE)
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 9)
        pdf.drawString(left_x + 36, y + 29, title)
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 7.4)
        pdf.drawRightString(left_x + 322, y + 29, detail)
        y -= 63
    draw_card(pdf, right_x, 128, 350, 330, fill=CHARCOAL, stroke=CHARCOAL)
    draw_label(pdf, "OPENGL 응용 계층", right_x + 18, 426, GOLD)
    gl_nodes = [
        ("Application", "프레임 처리 / 오류 UI"),
        ("ViewerUi", "입력 점유 / 명시적 동작"),
        ("GpuMesh", "이동 전용 VAO / VBO / EBO"),
        ("렌더러", "후보 프로그램 / 마지막 정상 셰이더"),
    ]
    y = 350
    for title, detail in gl_nodes:
        pdf.setFillColor(HexColor("#15363F"))
        pdf.roundRect(right_x + 20, y, 310, 51, 8, fill=1, stroke=0)
        pdf.setFillColor(GOLD)
        pdf.setFont("MalgunBold", 9)
        pdf.drawString(right_x + 36, y + 29, title)
        pdf.setFillColor(HexColor("#B9D2D5"))
        pdf.setFont("Malgun", 7.4)
        pdf.drawRightString(right_x + 312, y + 29, detail)
        y -= 63
    pdf.setStrokeColor(CYAN)
    pdf.setLineWidth(3)
    pdf.line(407, 292, 438, 292)
    pdf.setFillColor(CYAN)
    pdf.setFont("MalgunBold", 7)
    pdf.drawCentredString(422, 306, "검증된 데이터")
    draw_card(pdf, MARGIN, 65, 753, 44, fill=PALE)
    pdf.setFillColor(INK)
    pdf.setFont("Malgun", 8.2)
    pdf.drawCentredString(PAGE_WIDTH / 2, 83, "핵심 원칙 · GL 컨텍스트 없이 기하/컴파일러를 테스트하고, OpenGL 핸들은 이동 전용 RAII 객체만 소유")
    pdf.showPage()


def page_rendering(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 5, "04 · 렌더링", "인덱스 메시 렌더링 파이프라인", "VAO / VBO / EBO · GLSL · Blinn-Phong")
    nodes = [
        ("메시 파일", "STL / OBJ"),
        ("CPU 메시", "정점 / 인덱스"),
        ("GPU 메시", "VAO / VBO / EBO"),
        ("정점 셰이더", "MVP / 법선 행렬"),
        ("래스터화", "인덱스 삼각형"),
        ("프래그먼트 셰이더", "솔리드 / 법선 / 클리핑"),
        ("프레임버퍼", "뷰어 영역"),
    ]
    draw_pipeline(pdf, nodes, MARGIN, 365, PAGE_WIDTH - 2 * MARGIN, height=65)
    draw_image_fit(pdf, screenshots / "02_wireframe.png", MARGIN, 91, 354, 214)
    draw_image_fit(pdf, screenshots / "01_overview.png", 443, 91, 354, 214)
    draw_label(pdf, "와이어프레임", MARGIN + 12, 278)
    draw_label(pdf, "BLINN-PHONG 솔리드", 455, 278, GOLD)
    draw_card(pdf, MARGIN, 318, 753, 31, fill=PALE)
    pdf.setFillColor(INK)
    pdf.setFont("Malgun", 8)
    pdf.drawCentredString(PAGE_WIDTH / 2, 329, "CPU MeshData는 파일/기하 검증에 재사용 · GPU 전송은 렌더링 생명주기에 한정 · Uniform 위치는 프로그램별 캐시")
    pdf.showPage()


def page_camera(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 6, "05 · 카메라", "카메라와 치과 메시 좌표 규약", "회전 / 이동 / 확대·축소 · 경계 기반 맞춤")
    draw_image_fit(pdf, screenshots / "01_overview.png", MARGIN, 151, 430, 316)
    right_x = 500
    items = [
        ("회전", "중심점을 기준으로 Yaw/Pitch를 갱신하고 Pitch를 제한"),
        ("이동", "View의 Right/Up 방향으로 중심점과 Eye를 함께 이동"),
        ("확대·축소", "거리를 곱셈형으로 조절하고 안정 범위를 유지"),
        ("화면 맞춤", "월드 경계를 기준으로 거리와 Near/Far를 재계산"),
    ]
    y = 391
    for title, detail in items:
        draw_card(pdf, right_x, y, 297, 63, fill=white)
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 9)
        pdf.drawString(right_x + 15, y + 37, title)
        draw_paragraph(pdf, detail, right_x + 73, y + 37, 207, size=7.7, leading=11, color=MUTED)
        y -= 74
    draw_card(pdf, MARGIN, 73, 753, 59, fill=CHARCOAL, stroke=CHARCOAL)
    pdf.setFillColor(CYAN)
    pdf.setFont("MalgunBold", 8.5)
    pdf.drawString(MARGIN + 18, 108, "좌표 규약")
    pdf.setFillColor(white)
    pdf.setFont("Malgun", 8)
    pdf.drawString(MARGIN + 18, 89, "뷰어 내부 마우스 → NDC → Projection/View 역변환 → 월드 광선 · STL/OBJ 실제 단위는 추론하지 않고 모델 단위로 표시")
    pdf.showPage()


def page_picking(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 7, "06 · 기하 연산", "광선 피킹과 3D 직선거리", "최근접 교차점 · Möller-Trumbore · 모델 단위")
    draw_image_fit(pdf, screenshots / "03_picking.png", MARGIN, 230, 353, 237)
    draw_image_fit(pdf, screenshots / "04_measurement.png", 444, 230, 353, 237)
    draw_label(pdf, "최근접 교차점", MARGIN + 12, 438)
    draw_label(pdf, "두 점 거리 측정", 456, 438, GOLD)
    nodes = [
        ("마우스", "뷰어 좌표"),
        ("월드 광선", "View-Projection 역변환"),
        ("AABB", "빠른 탈락"),
        ("삼각형", "가장 가까운 t"),
        ("표면점", "월드 위치"),
        ("거리", "|B - A|"),
    ]
    draw_pipeline(pdf, nodes, MARGIN, 133, PAGE_WIDTH - 2 * MARGIN, height=59)
    draw_card(pdf, MARGIN, 66, 753, 49, fill=PALE)
    pdf.setFillColor(INK)
    pdf.setFont("MalgunBold", 8.5)
    pdf.drawString(MARGIN + 16, 94, "정확한 표현")
    pdf.setFont("Malgun", 8)
    pdf.drawString(MARGIN + 100, 94, "두 표면점 사이의 3차원 유클리드 직선거리이며, 표면을 따라가는 최단거리가 아닙니다.")
    pdf.showPage()


def page_clipping(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 8, "07 · 상호작용", "클리핑 미리보기와 뷰어 UI", "모델 좌표 평면 · 프래그먼트 폐기")
    draw_image_fit(pdf, screenshots / "05_clipping.png", MARGIN, 118, 470, 349)
    right_x = 545
    draw_card(pdf, right_x, 337, 252, 130, fill=CHARCOAL, stroke=CHARCOAL)
    pdf.setFillColor(CYAN)
    pdf.setFont("ConsolasBold", 12)
    pdf.drawString(right_x + 18, 425, "plane: dot(n, p) - d")
    pdf.setFillColor(white)
    pdf.setFont("Consolas", 9)
    pdf.drawString(right_x + 18, 397, "if (signedDistance > 0.0)")
    pdf.drawString(right_x + 32, 378, "discard;")
    pdf.setFillColor(HexColor("#9FC5C8"))
    pdf.setFont("Malgun", 7.2)
    pdf.drawString(right_x + 18, 354, "평면은 모델 좌표에 고정")
    draw_card(pdf, right_x, 192, 252, 125, fill=white)
    draw_label(pdf, "UI 입력 규약", right_x + 14, 286)
    draw_bullets(
        pdf,
        [
            "ImGui가 입력을 소비하면 카메라/피킹으로 전달하지 않음",
            "상태 변경과 오류를 콘솔뿐 아니라 뷰어 UI에도 표시",
            "카메라 이동과 클리핑 평면 상태를 분리",
        ],
        right_x + 14,
        260,
        224,
        size=7.5,
        leading=11,
    )
    draw_card(pdf, right_x, 91, 252, 81, fill=HexColor("#FFF4DC"), stroke=HexColor("#EACB8B"))
    pdf.setFillColor(RED)
    pdf.setFont("MalgunBold", 8.5)
    pdf.drawString(right_x + 14, 145, "한계")
    draw_paragraph(
        pdf,
        "단면 메시나 덮개를 생성하지 않는 시각적 미리보기입니다. 의료용 단면 생성 기능으로 표현하지 않습니다.",
        right_x + 14,
        124,
        224,
        size=7.5,
        leading=12,
    )
    pdf.showPage()


def page_minishader(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 9, "08 · MINISHADER", "작은 재질 DSL", "전체 GLSL이 아닌 의도적으로 제한된 표현 계층")
    draw_image_fit(pdf, screenshots / "06_minishader.png", MARGIN, 228, 410, 239)
    code_x = 480
    draw_card(pdf, code_x, 228, 317, 239, fill=CHARCOAL, stroke=CHARCOAL)
    pdf.setFillColor(CYAN)
    pdf.setFont("ConsolasBold", 8.5)
    pdf.drawString(code_x + 18, 438, "material Dental {")
    source_lines = [
        "  let n = normalize(normal);",
        "  let l = normalize(lightDir);",
        "  let diffuse = max(dot(n, l), 0.0);",
        "  let intensity = 0.25 + diffuse;",
        "  output = baseColor * intensity;",
        "}",
    ]
    pdf.setFillColor(white)
    pdf.setFont("Consolas", 8.3)
    y = 413
    for line in source_lines:
        pdf.drawString(code_x + 18, y, line)
        y -= 23
    cards = [
        ("작게 제한한 이유", "재질 계산에 필요한 스칼라/vec3와 허용 목록 함수만 지원"),
        ("명시적 적용 이유", "편집 중 자동 반영 대신 '컴파일 및 적용' 버튼으로 교체 시점 고정"),
        ("GLSL 생성 이유", "검증된 AST를 GLSL로 생성하고 최종 검증은 OpenGL 드라이버에 위임"),
    ]
    x = MARGIN
    for title, detail in cards:
        draw_card(pdf, x, 99, 239, 96, fill=white)
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 8.5)
        pdf.drawString(x + 15, 169, title)
        draw_paragraph(pdf, detail, x + 15, 146, 209, size=7.4, leading=12, color=MUTED)
        x += 257
    pdf.showPage()


def page_compiler(pdf: canvas.Canvas, screenshots: Path) -> None:
    begin_page(pdf, 10, "09 · 안전한 실행 환경", "컴파일러 파이프라인과 마지막 정상 셰이더", "오류가 렌더링 상태를 손상시키지 않도록")
    nodes = [
        ("어휘 분석", "토큰 + 행/열"),
        ("구문 분석", "AST + 연산 우선순위"),
        ("의미 분석", "타입 / 기호 / 인자 수"),
        ("GLSL", "제한된 생성기"),
        ("후보", "컴파일 + 링크"),
        ("교체", "성공할 때만 반영"),
    ]
    draw_pipeline(pdf, nodes, MARGIN, 386, PAGE_WIDTH - 2 * MARGIN, height=65)
    draw_image_fit(pdf, screenshots / "07_minishader_error.png", MARGIN, 101, 405, 248)
    right_x = 478
    draw_card(pdf, right_x, 234, 319, 115, fill=CHARCOAL, stroke=CHARCOAL)
    pdf.setFillColor(RED)
    pdf.setFont("MalgunBold", 9)
    pdf.drawString(right_x + 18, 320, "알 수 없는 식별자: missing.")
    pdf.setFillColor(HexColor("#B9D2D5"))
    pdf.setFont("Malgun", 8)
    pdf.drawString(right_x + 18, 294, "후보 생성 중단 · 현재 프로그램은 변경하지 않음")
    pdf.setFillColor(GOLD)
    pdf.setFont("MalgunBold", 10)
    pdf.drawString(right_x + 18, 263, "마지막 정상 셰이더를 그대로 유지")
    draw_card(pdf, right_x, 101, 319, 112, fill=white)
    draw_bullets(
        pdf,
        [
            "최대 소스 65,536바이트 · 표현식 중첩 128단계",
            "오류 행/열과 진단 내용을 UI에 표시",
            "성공한 OpenGL 프로그램만 이동 대입으로 교체",
        ],
        right_x + 16,
        181,
        286,
        size=7.8,
        leading=12,
    )
    pdf.showPage()


def page_quality(pdf: canvas.Canvas) -> None:
    begin_page(pdf, 11, "10 · 품질", "개발 품질과 재현성", "테스트 · CI · 벤치마크 · Windows 릴리스")
    draw_metric(pdf, MARGIN, 366, 170, "63 / 63", "Debug / Release 테스트", "Catch2 · /W4 /WX")
    draw_metric(pdf, 229, 366, 170, "1,246", "벤치마크 원시 표본", "10만 / 50만 삼각형")
    draw_metric(pdf, 416, 366, 170, "0.516 ms", "50만 CPU 프레임 중앙값", "1280×720 · VSync 해제")
    draw_metric(pdf, 603, 366, 194, "3.321 ms", "50만 피킹 중앙값", "AABB + 전체 삼각형")
    left_x, right_x = MARGIN, 429
    draw_card(pdf, left_x, 116, 345, 223, fill=white)
    draw_label(pdf, "방어적 설계", left_x + 18, 308)
    draw_bullets(
        pdf,
        [
            "이동 전용 RAII로 VAO/VBO/EBO/프로그램 핸들 수명 한정",
            "NaN, 손상 메시, 과대 소스, 깊은 중첩 입력을 경계에서 거부",
            "실제 GPU에서 잘못된 GLSL과 마지막 정상 셰이더 경로 검증",
            "Windows 패키지에 셰이더, README, 라이선스 17건을 함께 배포",
        ],
        left_x + 18,
        276,
        310,
        size=8.2,
        leading=13,
    )
    draw_card(pdf, right_x, 116, 368, 223, fill=CHARCOAL, stroke=CHARCOAL)
    draw_label(pdf, "재현 가능한 배포", right_x + 18, 308, GOLD)
    rows = [
        ("빌드", "CMakePresets + vcpkg Manifest + MSVC"),
        ("CI", "GitHub Actions Windows Debug/Release"),
        ("성능", "CSV 원시 표본 + 측정 방법 + 요약"),
        ("패키지", "외부 작업 폴더에서 ZIP 스모크 테스트"),
        ("근거", "커밋별 docs/verification 기록"),
    ]
    y = 276
    for label, detail in rows:
        pdf.setFillColor(GOLD)
        pdf.setFont("MalgunBold", 8.3)
        pdf.drawString(right_x + 18, y, label)
        pdf.setFillColor(HexColor("#C3DADD"))
        pdf.setFont("Malgun", 7.7)
        pdf.drawString(right_x + 79, y, detail)
        y -= 33
    pdf.setFillColor(MUTED)
    pdf.setFont("Malgun", 7.3)
    pdf.drawString(MARGIN, 88, "측정 환경: NVIDIA GeForce RTX 5070 Ti · OpenGL 3.3 · 릴리스 · 2026-08-17")
    pdf.linkURL(CI_URL, (right_x + 18, 116, right_x + 350, 330), relative=0)
    pdf.showPage()


def page_career(pdf: canvas.Canvas, applicant: str) -> None:
    begin_page(pdf, 12, "11 · 경력 연결", "경력 연결과 제출 링크", "C++ 상용 소프트웨어 경험을 3D 그래픽스로 확장")
    pathway = [
        ("C++ / MFC", "상용 Windows 소프트웨어"),
        ("GDI+", "2D 드로잉"),
        ("MapLibre", "지도 렌더링"),
        ("Fabric.js", "대화형 캔버스"),
        ("OpenGL / GLSL", "3D 파이프라인"),
        ("치과 3D", "뷰어 상호작용"),
    ]
    draw_pipeline(pdf, pathway, MARGIN, 386, PAGE_WIDTH - 2 * MARGIN, height=66)
    draw_card(pdf, MARGIN, 109, 432, 236, fill=white)
    draw_label(pdf, "직무 근거", MARGIN + 18, 314)
    evidence = [
        ("C/C++", "C++20 핵심부, 이동 전용 RAII, CMake/MSVC"),
        ("OpenGL", "3.3 Core, VAO/VBO/EBO, GLSL, 피킹"),
        ("아키텍처", "OpenGL 비의존 핵심부 / GL 경계 / 마지막 정상 셰이더"),
        ("상용 소프트웨어", "명시적 오류·입력 경계·Windows 패키지"),
        ("CI/CD", "GitHub Actions + 재현 가능한 릴리스 스크립트"),
    ]
    y = 278
    for requirement, proof in evidence:
        pdf.setFillColor(TEAL)
        pdf.setFont("MalgunBold", 8.5)
        pdf.drawString(MARGIN + 18, y, requirement)
        pdf.setFillColor(MUTED)
        pdf.setFont("Malgun", 8)
        pdf.drawString(MARGIN + 112, y, proof)
        y -= 34
    draw_qr_link(pdf, REPOSITORY_URL, "GitHub 저장소", 534, 193, 96)
    draw_qr_link(pdf, DEMO_URL, "84초 시연", 672, 193, 96)
    draw_link(pdf, "GitHub", REPOSITORY_URL, 518, 147, 270)
    draw_link(
        pdf,
        "시연 영상",
        DEMO_URL,
        518,
        105,
        270,
        "github.com/leeleemaster/OpenGL_MSVC / docs/demo / DentalViz-v0.8-portfolio-demo.mp4",
    )
    pdf.setFillColor(MUTED)
    pdf.setFont("Malgun", 7.8)
    pdf.drawRightString(PAGE_WIDTH - MARGIN, 76, f"포트폴리오 식별자: {applicant}")
    pdf.showPage()


def build_pdf(repository_root: Path, output: Path, applicant: str) -> None:
    register_fonts()
    output.parent.mkdir(parents=True, exist_ok=True)
    screenshots = repository_root / "docs" / "screenshots"
    required = [screenshots / f"0{index}_{name}.png" for index, name in [
        (1, "overview"),
        (2, "wireframe"),
        (3, "picking"),
        (4, "measurement"),
        (5, "clipping"),
        (6, "minishader"),
        (7, "minishader_error"),
    ]]
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise FileNotFoundError("Missing portfolio screenshots: " + ", ".join(missing))

    pdf = canvas.Canvas(str(output), pagesize=(PAGE_WIDTH, PAGE_HEIGHT), pageCompression=1)
    pdf.setTitle("DentalViz - C++ 기반 치과 3D 시각화와 MiniShader 실행 환경")
    pdf.setAuthor(applicant)
    pdf.setSubject("휴비츠 3D 그래픽스 개발자 포트폴리오")
    pdf.setKeywords("C++, OpenGL, 치과, 3D 뷰어, MiniShader, MSVC")

    page_cover(pdf, screenshots, applicant)
    page_background(pdf)
    page_features(pdf, screenshots)
    page_architecture(pdf)
    page_rendering(pdf, screenshots)
    page_camera(pdf, screenshots)
    page_picking(pdf, screenshots)
    page_clipping(pdf, screenshots)
    page_minishader(pdf, screenshots)
    page_compiler(pdf, screenshots)
    page_quality(pdf)
    page_career(pdf, applicant)
    pdf.save()


def parse_arguments() -> argparse.Namespace:
    repository_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="DentalViz 12페이지 포트폴리오 PDF를 생성합니다.")
    parser.add_argument("--applicant", default="leeleemaster")
    parser.add_argument(
        "--output",
        type=Path,
        default=repository_root / "output" / "pdf" / "DentalViz_Huvitz_Portfolio.pdf",
    )
    return parser.parse_args()


if __name__ == "__main__":
    arguments = parse_arguments()
    root = Path(__file__).resolve().parent.parent
    build_pdf(root, arguments.output.resolve(), arguments.applicant)
    print(arguments.output.resolve())
