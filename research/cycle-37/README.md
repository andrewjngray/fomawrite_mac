# Cycle 37 — output templates and portable images

Output style selection and custom template font/size/header/footer/title-page settings persist. PDF/print share paginated layout with optional document headers and page counters. Insert Page Break adds an ordinary Markdown comment interpreted only for output. HTML embeds local raster images (5 MiB each / 20 MiB total); unreadable/missing/remote images reject the export rather than claiming portability. Existing output is retained on failure.

Build and 65 tests pass. Tests cover persisted styles, embedded PNG data, page-break marker removal, PDF generation and failed export preservation. PDF skill rendered the synthetic two-page PDF. Visual inspection caught an image moving across a page break; the corrected output-final pages keep the image on page 1, text on page 2, readable margins/header/footer. Only output-final is final evidence. Native export/picker checks occur in the combined run.

Limits: no in-app paginated/fit-page preview; external document hyperlinks remain external references. Imported templates are declarative, not CSS engines. Custom title-page layout and physical-printer behavior need broader native QA.
