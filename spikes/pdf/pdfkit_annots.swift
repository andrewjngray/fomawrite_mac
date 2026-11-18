// Adds Preview-style annotations with PDFKit (the framework Preview uses) to
// page 1 of geometry.pdf. Usage: swift pdfkit_annots.swift <geometry.pdf> <out.pdf>
import Foundation
import PDFKit
import AppKit

let args = CommandLine.arguments
guard args.count == 3, let doc = PDFDocument(url: URL(fileURLWithPath: args[1])) else { fatalError("usage") }
while doc.pageCount > 1 { doc.removePage(at: 1) }
let page = doc.page(at: 0)!
func word(_ w: String) -> PDFSelection { page.selection(for: page.bounds(for: .mediaBox)).flatMap { _ in doc.findString(w, withOptions: []).first }! }
func markup(_ sel: PDFSelection, _ type: PDFAnnotationSubtype, _ colour: NSColor) {
    let a = PDFAnnotation(bounds: sel.bounds(for: page), forType: type, withProperties: nil)
    let b = sel.bounds(for: page)
    a.quadrilateralPoints = [NSValue(point: NSPoint(x: 0, y: b.height)), NSValue(point: NSPoint(x: b.width, y: b.height)),
                             NSValue(point: NSPoint(x: 0, y: 0)), NSValue(point: NSPoint(x: b.width, y: 0))]
    a.color = colour
    page.addAnnotation(a)
}
markup(word("ALPHA"), .highlight, NSColor(calibratedRed: 1, green: 0.85, blue: 0, alpha: 1))
markup(word("OMEGA"), .underline, NSColor.systemBlue)
markup(word("OMEGA"), .strikeOut, NSColor.systemRed)
let note = PDFAnnotation(bounds: NSRect(x: 450, y: 650, width: 20, height: 20), forType: .text, withProperties: nil)
note.contents = "Spike note"; note.color = NSColor.systemYellow
page.addAnnotation(note)
print(doc.write(to: URL(fileURLWithPath: args[2])) ? "wrote \(args[2])" : "write failed")
for a in page.annotations { print(a.type ?? "?", a.bounds) }
