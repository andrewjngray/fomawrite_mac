# Output Style menu

**Output Style** means the appearance used for the rendered preview and published Web/PDF output. It does not change the Markdown source, the Visual Edit mode, or the editor’s Writing Appearance. HTML and PDF use this publishing style; the Export and Share panel also lets you choose the output format and PDF page setup.

## Choose a style

The menu’s **Custom Themes** submenu sits above Modern (Sans) and lists available CSS publishing themes, including the bundled Claude Like theme. Hover or click **Custom Themes ›** to open it; long theme lists scroll. Selecting one applies its CSS to Web and PDF output and remembers it. If a theme selection is rejected, an error explains the problem and the previously accepted style stays selected.

The basic font and page presets remain in the main menu:

- **Modern (Sans)**, **Classic (Serif)** and **Manuscript (Mono)** are the default basic styles. They request Helvetica Neue, Georgia and iA Writer Mono S respectively, at 12 pt. Manuscript uses double line spacing; Modern and Classic use 1.5. Available fonts can affect the result.
- **GitHub**, **Helvetica**, **Palatino** and **MLA Draft** are also basic styles. GitHub currently uses Helvetica Neue and the shared basic layout; Helvetica and Palatino use their named fonts. MLA Draft uses Times New Roman, double spacing, indented body paragraphs and centered, plain first-level headings; it does not claim full MLA compliance.
- **Custom Settings** selects the editable basic style. Its saved font, point size, header/footer and page-furniture options apply to PDF page layout; a title page can also be enabled. The initial custom font is Georgia at 12 pt. Choosing another basic style stops using these custom settings without deleting them.

The footer button stays labelled **Output Style**. Its hover hint names the selected style, and the menu checkmark identifies it. The selected style is remembered between launches. Choosing a basic style clears the selected CSS theme. An optional **Additional publishing CSS** file, chosen in Export and Share, is layered over the selected style and can be cleared there.

## Manage themes and custom settings

- **Get More Themes…** opens the Typora theme gallery in your browser. It does not download or install a theme automatically.
- **Import Theme…** imports a local `.css` theme and its supported local resources, copies it into Fomawrite’s publishing-themes folder, then selects it. Imported CSS and bundled resources are used for Web and PDF output.
- **Open Themes Folder** opens that app-data folder (creating it if needed). `.css` files in the folder are discovered as themes; nested folders are watched for changes to resources referenced by imported themes. On this Mac the folder is `~/Library/Application Support/AndrewGray/fomawrite/publishing-themes/`. Keep top-level CSS files with their matching resource subfolders.
- **Reload Themes** rescans the folder and refreshes the preview. Folder changes are also watched automatically. Reload does not change the document.
- **Edit Basic Settings…** opens Export and Share at PDF setup, where you can edit basic font/page settings and inspect the rendered output. Opening the panel alone does not select Custom Settings or open a CSS editor.
- **Load Custom Settings…** loads a local JSON font-and-page settings file, switches to Custom Settings, and saves those settings for later. It is separate from importing a CSS theme. A minimal file is `{"fontFamily":"Georgia","pointSize":14}`; optional fields are `header`, `footer`, `titlePage` and `pageFurniture`.

## PDF and HTML setup

In Export and Share, the format selector chooses PDF or HTML. Both use the selected output style. Paper size and orientation affect PDF pagination (and print layout); available paper choices are A4, US Letter, US Legal and A5. HTML uses the style too, and paper settings apply when printing HTML. The panel’s separate **Choose CSS…** action adds local CSS on top of the chosen style; **Clear** removes that extra CSS.

## Naming

**Output Style** is the clearest menu title: it covers CSS themes and basic font/page presets. **Output Format** would suggest PDF or HTML, which is selected separately. “Publishing Theme” is too narrow because most choices are not CSS themes.

One naming overlap remains: **GitHub** in the basic presets is a built-in basic style, while importing a file named GitHub can create a separate CSS theme in **Custom Themes**. The submenu label **Custom Themes** helps distinguish the latter, but the names may still be confusing when both are installed.
