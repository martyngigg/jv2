# jv2
Viewer for data archive journals

## Quick User Guide

 - Click and drag column headings to re-order your table view.
 - use the options in "View" to:
    - modify the visible columns
    - save your preferences so you always can open JournalViewer to your prefered view

 - Right click on a run or a selected series of runs to access a context menu
 - This context menu will provide you with a series of options to visualise your selected runs
 - Within a graph:
    - Use the arrow keys or hold in the mouse wheel to navigate your view
    - Use the mouse wheel or click and drag to zoom in/ out
    - Hold control to drag-zoom in the y- axis
    - Right click to reset the view

 - In the table view:
    - Use CTRL + G to toggle data grouping
    - Look under "Find" for an array of search shortcuts
    - Use CTRL + R to check for any new runs
    - Use CTRL + SHIFT + F to clear searches

## Development

### Linux

This guide assumes a Linux distribution with `curl` and `gpg` installed
along with a clone of this repository.
The following commands should be executed from the root of the clone.

First install the nix package manager (this needs to be done only once):

```sh
>./tools/development/install_and_configure_nix.sh
```

Next build pre-requisites and frontend source code:

```sh
>nix build -L .#jv2
```

Now run Journal Viewer 2:

```
>python3 backend/isisInternal.py
>result/bin/jv2
```
