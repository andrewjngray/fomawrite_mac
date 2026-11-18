#!/usr/bin/env python3
"""Write the Phase 0 spike fixtures: small hand-built PDFs whose text sits at
known PDF user-space positions, so geometry and rendering can be checked
exactly. Usage: make_fixtures.py <output dir>

geometry.pdf   5 pages, each with ALPHA at (100, 600) and OMEGA at (300, 200),
               Helvetica 24: page 1 /Rotate 0, 2 /Rotate 90, 3 /Rotate 180,
               4 /Rotate 270, 5 /Rotate 0 with CropBox [50 100 562 742].
annots.pdf     geometry.pdf page 1 with a yellow Highlight over ALPHA, an
               Underline and a StrikeOut over OMEGA and a Text (sticky note)
               annotation, none with an appearance stream (/AP).
annots-ap.pdf  the same annotations, each with a simple appearance stream.
pages-300.pdf  300 text pages, "Page N" at a known position.
images-50mb.pdf 10 pages, each a 1600x1000 RGB noise image (~48 MB total).
"""
import os
import random
import sys
import zlib

ALPHA = (100, 600)
OMEGA = (300, 200)
FONT_SIZE = 24
# Helvetica advance widths (1/1000 em) for the capitals used here.
WIDTHS = {"A": 667, "L": 556, "P": 667, "H": 722, "O": 778, "M": 833, "E": 667, "G": 778}


def text_width(word):
    return sum(WIDTHS[c] for c in word) * FONT_SIZE / 1000


def quad(word, origin):
    """QuadPoints for a word: x1 y1 x2 y2 x3 y3 x4 y4 (top-left, top-right,
    bottom-left, bottom-right), from descender to cap height."""
    x, y = origin
    w = text_width(word)
    top, bottom = y + 0.718 * FONT_SIZE, y - 0.207 * FONT_SIZE
    return [x, top, x + w, top, x, bottom, x + w, bottom]


class Pdf:
    def __init__(self):
        self.objects = []

    def add(self, body):
        self.objects.append(body)
        return len(self.objects)

    def stream(self, data, extra=""):
        if isinstance(data, str):
            data = data.encode("latin-1")
        return self.add(b"<< /Length %d %s >>\nstream\n" % (len(data), extra.encode()) + data + b"\nendstream")

    def write(self, path, root):
        out = bytearray(b"%PDF-1.7\n%\xe2\xe3\xcf\xd3\n")
        offsets = []
        for number, body in enumerate(self.objects, 1):
            offsets.append(len(out))
            if isinstance(body, str):
                body = body.encode("latin-1")
            out += b"%d 0 obj\n" % number + body + b"\nendobj\n"
        xref = len(out)
        out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(self.objects) + 1)
        for offset in offsets:
            out += b"%010d 00000 n \n" % offset
        out += b"trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(self.objects) + 1, root, xref)
        with open(path, "wb") as f:
            f.write(out)


def word_ops(word, origin):
    return f"BT /F1 {FONT_SIZE} Tf {origin[0]} {origin[1]} Td ({word}) Tj ET\n"


def build(path, pages, annotations_for=None, appearance=False):
    pdf = Pdf()
    font = pdf.add("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
    page_ids = []
    pages_id = len(pdf.objects) + 2 * len(pages) + 1  # placeholder, fixed below
    page_bodies = []
    for index, spec in enumerate(pages):
        content = pdf.stream(spec["content"])
        annots = ""
        if annotations_for and index in annotations_for:
            ids = [make_annotation(pdf, a, appearance) for a in annotations_for[index]]
            annots = "/Annots [%s]" % " ".join(f"{i} 0 R" for i in ids)
        page_bodies.append((content, spec, annots))
    for content, spec, annots in page_bodies:
        extra = f"/Rotate {spec.get('rotate', 0)}" + (f" /CropBox [{' '.join(map(str, spec['crop']))}]" if spec.get("crop") else "")
        page_ids.append(pdf.add(f"<< /Type /Page /Parent PAGES /MediaBox [0 0 612 792] {extra} "
                                f"/Resources << /Font << /F1 {font} 0 R >> {spec.get('xobjects', '')} >> "
                                f"/Contents {content} 0 R {annots} >>"))
    pages_id = pdf.add("<< /Type /Pages /Kids [%s] /Count %d >>" % (" ".join(f"{i} 0 R" for i in page_ids), len(page_ids)))
    for i in page_ids:
        pdf.objects[i - 1] = pdf.objects[i - 1].replace("/Parent PAGES", f"/Parent {pages_id} 0 R")
    root = pdf.add(f"<< /Type /Catalog /Pages {pages_id} 0 R >>")
    pdf.write(path, root)


def make_annotation(pdf, spec, appearance):
    kind, colour = spec["kind"], spec["colour"]
    q = spec.get("quads", [])
    rect = spec.get("rect") or [min(q[0::2]), min(q[1::2]), max(q[0::2]), max(q[1::2])]
    body = f"<< /Type /Annot /Subtype /{kind} /Rect [{' '.join(f'{v:.2f}' for v in rect)}] /C [{colour}] /F 4 "
    if q:
        body += f"/QuadPoints [{' '.join(f'{v:.2f}' for v in q)}] "
    if kind == "Text":
        body += "/Contents (Spike note) /Name /Note /Open false "
    if appearance:
        w, h = rect[2] - rect[0], rect[3] - rect[1]
        if kind == "Highlight":
            ops = f"/GS0 gs {colour} rg 0 0 {w:.2f} {h:.2f} re f"
        elif kind == "Underline":
            ops = f"{colour} RG 1.5 w 0 1 m {w:.2f} 1 l S"
        elif kind == "StrikeOut":
            ops = f"{colour} RG 1.5 w 0 {h / 2:.2f} m {w:.2f} {h / 2:.2f} l S"
        else:
            ops = f"{colour} rg 0 0 {w:.2f} {h:.2f} re f"
        gs = "/ExtGState << /GS0 << /Type /ExtGState /CA 0.4 /ca 0.4 /BM /Multiply >> >>" if kind == "Highlight" else ""
        ap = pdf.stream(ops, f"/Type /XObject /Subtype /Form /BBox [0 0 {w:.2f} {h:.2f}] /Resources << {gs} >>")
        body += f"/AP << /N {ap} 0 R >> "
    return pdf.add(body + ">>")


def geometry_pages():
    base = word_ops("ALPHA", ALPHA) + word_ops("OMEGA", OMEGA)
    return [{"content": base, "rotate": r} for r in (0, 90, 180, 270)] + \
           [{"content": base, "rotate": 0, "crop": [50, 100, 562, 742]}]


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    build(os.path.join(out, "geometry.pdf"), geometry_pages())
    marks = {0: [
        {"kind": "Highlight", "colour": "1 0.85 0", "quads": quad("ALPHA", ALPHA)},
        {"kind": "Underline", "colour": "0 0.4 1", "quads": quad("OMEGA", OMEGA)},
        {"kind": "StrikeOut", "colour": "1 0 0", "quads": quad("OMEGA", OMEGA)},
        {"kind": "Text", "colour": "1 0.9 0.3", "rect": [450, 650, 470, 670]},
    ]}
    page = geometry_pages()[:1]
    build(os.path.join(out, "annots.pdf"), page, marks, appearance=False)
    build(os.path.join(out, "annots-ap.pdf"), page, marks, appearance=True)
    build(os.path.join(out, "pages-300.pdf"),
          [{"content": word_ops("PAGE", (72, 720)) + f"BT /F1 24 Tf 160 720 Td ({n}) Tj ET\n"} for n in range(1, 301)])
    rng = random.Random(158)
    images = []
    for n in range(10):
        raw = bytes(rng.getrandbits(8) for _ in range(1600 * 1000 * 3))
        images.append(zlib.compress(raw, 1))
    pdf_pages = []
    for n, data in enumerate(images):
        pdf_pages.append({"content": f"q 612 0 0 383 0 300 cm /Im{n} Do Q\n" + word_ops("PAGE", (72, 720)),
                          "image": data, "n": n})
    build_images(os.path.join(out, "images-50mb.pdf"), pdf_pages)
    for name in sorted(os.listdir(out)):
        print(f"{name}: {os.path.getsize(os.path.join(out, name)):,} bytes")


def build_images(path, pages):
    pdf = Pdf()
    font = pdf.add("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
    kids = []
    for spec in pages:
        image = pdf.stream(spec["image"], "/Type /XObject /Subtype /Image /Width 1600 /Height 1000 "
                                          "/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /FlateDecode")
        content = pdf.stream(spec["content"])
        kids.append(pdf.add(f"<< /Type /Page /Parent PAGES /MediaBox [0 0 612 792] /Resources << /Font << /F1 {font} 0 R >> "
                            f"/XObject << /Im{spec['n']} {image} 0 R >> >> /Contents {content} 0 R >>"))
    pages_id = pdf.add("<< /Type /Pages /Kids [%s] /Count %d >>" % (" ".join(f"{i} 0 R" for i in kids), len(kids)))
    for i in kids:
        pdf.objects[i - 1] = pdf.objects[i - 1].replace("/Parent PAGES", f"/Parent {pages_id} 0 R")
    root = pdf.add(f"<< /Type /Catalog /Pages {pages_id} 0 R >>")
    pdf.write(path, root)


if __name__ == "__main__":
    main()
