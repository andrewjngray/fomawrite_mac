#pragma once

// PDF page operations (Cycle 165), written over qpdf. This and pdfeditor.cpp
// are the only files that include qpdf headers. Every function is pure: bytes
// in, bytes out, no file access, no GUI; qpdf exceptions never escape and
// become ok=false with a plain-English message. Page numbers are 0-based.

#include <QByteArray>
#include <QString>
#include <QVector>

namespace PdfEditor {

// One page of an operation's output.
struct PageRef {
    int source;     // 0-based page of the INPUT this page came from; -1 for a page inserted from another document
    int rotation;   // clockwise change this operation applied: 0, 90, 180 or 270
};

struct Result {
    bool ok = false;
    QString error;              // plain English; empty when ok
    QByteArray bytes;           // the new document
    QVector<PageRef> pages;     // one entry per page of the OUTPUT, in order
};

enum class Refusal { None, Damaged, NeedsPassword, Signed };

// Why the document cannot be edited, or None. `message` receives a sentence for the user.
//  Damaged:       qpdf cannot read it, or it has no pages.
//  NeedsPassword: encrypted and qpdf cannot open it with an empty password, or it opens
//                 but its permissions forbid page changes (the owner password is needed).
//  Signed:        a signature field with a value, /SigFlags saying signatures exist, or /Perms.
// An encrypted PDF that opens with an empty user password and allows page changes is
// editable; the output keeps its encryption (extract() writes an unencrypted copy).
Refusal check(const QByteArray &pdf, QString *message = nullptr);

// Edits of the open document. Each refuses a Damaged or NeedsPassword input; a Signed input
// only when allowSigned is true (editing invalidates the signature). Everything else in
// the document (metadata, outlines, links, annotations on kept pages, page boxes) is left as qpdf keeps it.
//
// Selections are sets: order and repeats are ignored (except in extract). An empty selection or an
// index outside 0..pageCount-1 is an error (ok=false), never a silent no-op.
//  rotate:  degrees clockwise relative to each page's current (possibly inherited) /Rotate;
//           ±90, ±270 or 180. Output pages: all input pages, in order.
//  remove:  refuses to remove every page. Pages of the output are the survivors, in order.
//  move:    the selected pages move, in their original relative order, to just before input page
//           `before` (0..pageCount; pageCount = end). A move that changes nothing returns ok with the
//           input bytes untouched and an identity page list.
//  insert:  all pages of `other` go in before input page `before` (0..pageCount). `other` must itself be
//           readable and password-free; a signed `other` is accepted.
Result rotate(const QByteArray &pdf, const QVector<int> &pages, int degreesClockwise, bool allowSigned = false);
Result remove(const QByteArray &pdf, const QVector<int> &pages, bool allowSigned = false);
Result move(const QByteArray &pdf, const QVector<int> &pages, int before, bool allowSigned = false);
Result insert(const QByteArray &pdf, const QByteArray &other, int before, bool allowSigned = false);

// A new document holding only these pages, in the given order (repeats allowed). Metadata, outlines and
// forms are not carried over. The source is not modified, so a signed source needs no allow flag.
Result extract(const QByteArray &pdf, const QVector<int> &pages);

}
