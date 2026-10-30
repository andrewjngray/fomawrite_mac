# Fomawrite Windows — launch and update

## On the Windows VM

Open `C:\Mac\Home\repo\projects\fomawrite_windows` and double-click **Launch.cmd**.
Windows may hide the `.cmd` extension and show just **Launch**.

It checks the current downloaded build on every click, verifies it, copies it to
Windows-local storage when needed, and opens the app. Subsequent clicks bring the
same app forward. You can also drop a Markdown file onto Launch.cmd to open it.

## On the Mac

In this same project folder, double-click **Launch.command**. It checks GitHub for
the newest successful complete Windows build, downloads both architectures when
needed, starts Windows 11 in Parallels if necessary, and launches the ARM64 app.
If offline, it can use the last downloaded package after verifying it.

Double-click **Update.command** to update the downloaded builds without launching.
Windows Launch uses the local downloads; it does not sign in to GitHub or download
new builds itself. Run Update.command or Launch.command on the Mac to fetch them.

## Your documents

Choose your writing folder using the app's Folder button. Shared Mac documents
remain in their original folder; application updates do not copy or rewrite them.
The launcher preserves saved workspace state across updates and never overwrites
an existing workspace. Close an older running build normally before opening a
new one. Unsaved recovery files are left intact.

See [Windows status](docs/windows/STATUS.md) for the exact build and verification
limits. This is an internal portable Windows build, not a finished installer.
