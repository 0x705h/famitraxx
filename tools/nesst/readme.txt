NES Screen Tool by Shiru



This Windows tool allows to edit NES nametables, attributes, palettes,
pattern graphics, and metasprites. Unlike most of similar programs,
it is targeted to creating of content for new homebrew projects rather
than editing existing ones, although some of functionality of this kind
is also there.

The program and its source code are released into the Public Domain,
you can do whatever you want with it. The source code is for
Borland Turbo C++ Explorer (C++ Builder).



Tileset editor

LMB   - set tile as current
LMBx2 - open the tile in the CHR editor

Shift+LMB - select a rectangular group of tiles
Ctrl+LMB  - select random set of tiles
RMB       - cancel selection

Ctrl+Z   - undo
Ctrl+X   - cut tile graphics
Ctrl+C   - copy tile graphics
Ctrl+V   - paste tile graphics

When you copy random set of tiles, they will be pasted as a linear sequence.
When you copy rectangular group of tiles, they will be pasted as a rectangle
as well.

Only half of 8K CHR memory is visible at a time, you can switch between the
halfs using 'A' and 'B' buttons under the tileset.

'Apply tiles' button allows to change tile numbers in the nametable. You can't
draw anything in the nametable when this button is not pressed down.

'Attr checker' button allows to check nametable attributes with a special
checker tile. Warning, you still can draw tiles in this mode.

'Type in' button allows to enter a sequence of tiles into the nametable at
given position (click to set) either by clicking tiles in the tileset, or
using keyboard. In this case tileset should contain a font with ASCII encoding,
with space being tile 0.

'Grid All', '1x', '2x', '4x' buttons controls grid displaying. There are grids
of three sizes that could be used at once or in any combination.



Palette editor

LMB selects current color and palette, or assigns a color to the current color
in the current palette.

'Apply palettes' button enables applying current palette to the nametable.

'R0','G0','B0','M0' buttons controls the color emphasis bits that alters the
palette.



Nametable editor

Select nametable tab to enter the nametable editor.

You can draw nametable with a single tile when a single tile in the CHR is
selected, or with a rectangular brush when a rectangular group of tiles in
the CHR is selected.

LMB - set current tile or CHR selection into nametable
RMB - set the tile from the nametable as current, cancel nametable selection

Shift+LMB - select an area of the nametable
Caps Lock - switch between selection and drawing mode

Cursor      - rotate nametable without attributes with wrap around
Ctrl+Cursor - rotate nametable with attributes (2 chars per step)

Ctrl+F - fill selection with current tile
A      - enable/disable the attribute checker tile

Ctrl+Z - undo
Ctrl+X - cut selection
Ctrl+C - copy selecton
Ctrl+V - paste selection

Shift+C - copy selection into the Windows clipboard as text
Shift+M - copy selection as a metasprite into the Windows clipboard
Shift+N - copy selection as a flipped metasprite into the Windows clipboard



Metasprite editor

Select Metasprites tab to enter the metasprites editor.

You can drag and drop tiles from the tileset to metasprite editor using
right mouse button. You can move sprites around in the editor using RMB
or Cursor keys. To select a sprite you either can click it in the editor
with LMB, or in the list. When a sprite is selected, you can change tile
or palette assigned to it by clicking on the tileset or the palettes with
LMB.

You can change the origin of coordinates by clicking LMB+Ctrl in the
metasprite editor area. All sprite coordinates will be recalculated.
This is useful when you need to have origin at the bottom of metasprites,
for example.

You can edit up to 256 metasprites at once, up to 64 sprites per
metasprite.

'<' and '>' buttons selects a metasprite.

'Clear' button clears current metasprite.

'Copy' button copy current metasprite into internal clipboard

'Paste' button paste content of the internal clipboard into current metasprite

'HFlip' and 'VFlip' buttons flips current metasprite horizontally or vertically relatively to the origin of coordinates

'All', 'Sel', 'None' buttons allow to select which sprite will have a
frame displayed around it in the editor, all sprites, only selected one,
or none.

'Snap' button enables snap to the 8x8 grid when you move sprites around
with the mouse or the Cursor keys.

'Up' and 'Down' buttons move a sprite up or down in the list.

'H' and 'V' buttons toggle horizontal and vertical flip bits for
selected sprite.

'Del' deletes selected sprite from the list.


[ - select previous metasprite
] - select next metaspirte



Main menu

All

 Open

  Load and save tileset, nametable, and palette of the same name at once.

 Save

  Save tileset, nametable, and palette. Names will be asked for every file.

 Load session

  Load previously saved state of the editor.

 Save session

  Save current state of the editor, including all the resources, selections,
  internal clipboard, undo, and settings. It could be used when you need to
  save your work to continue later.


Patterns

 Open CHR

  Load CHR file with size of 1K, 2K, 4K, 8K, or other size of multiple of 16.
  4K files always replace current tileset, smaller files loads to the current
  tile position.

 Save CHR

  Save CHR file of one of standart sizes, similar to the open function.

 Save selection

  Save current selection from the tileset. They are not necessarily in
  order, you can select and save random set of tiles.

 Find doubles

  Find and select matching tiles in the tileset.

 Remove doubles

  Find and remove matching tiles. You can select an area in the tileset before
  using this function in order to exclude the area from the process. This way
  you can optimize the tileset without changing the selected area and keeping
  tiles in the area in place.

 Find unused
 
  Find and select unused tiles in the tileset.
  
 Remove unused
 
  Find and remove unused tiles, works similar to Remove doubles.
 
 Interleave
 Deinterleave

  Interleave and deinterleave tiles in the tileset to simplify work with 8x16
  sprites. Draw them as two tiles one above other, then use Interleave function
  of Patterns menu to rearrange the tiles in the order required by NES hardware
  (top become left, bottom become right). Use Deinterleave function if you need
  to rearrange the tiles back. Be careful, this function changes actual
  arrangement of the data, not just it appearance.

 Swap colors

  Swap colors in the graphics and/or in the palette. Shows a dialog with
  settings, you can swap the colors in whole tileset, in a bank, or in a
  selection.

 Swap banks

  Swap 4K parts (banks A and B) of the tileset.

 Clear

  Clear tileset.

 Fill with numbers

  You can fill the tileset with graphics of tile numbers (00..FF) instead of
  clearing it. This could help to track down sprites and tiles that are used
  in wrong places.


Nametable

 Open nametable

  Open a nametable, with or without attributes, RLE packed or not.

 Save nametable only
 Save nametable and attributes

  Save nametable with or without attributes data (960 or 1024 bytes). You can
  also save and load RLE-packed nametables with or without attributes.
  Decompressor code for NES is provided as well.

 Save attributes only

  Save attributes data only (64 bytes).

 Add offset

  Add a value to tile numbers in the nametable. A dialog with settings will
  appear, you can select range of tile numbers that should be affected by the
  function, and value to add (could be negative). All values are decimal,
  hex is also displayed.

 Put selection to clipboard as ASM

  Copy current selection from the nametable into the Windows clipboard as
  a series of db statements, to be pasted into assembly code.

 Put selection to clipboard as C (Shift+C)

  Copy current selection from the nametable into the Windows clipboard as
  an array definition, to be pasted into C code.

 Put metasprite as C (Shift+M)

  Copy current selection from the nametable into the Windows clipboard as
  an array definition in special format, to be pasted into C code. The format:

   X offset from the top left corner in pixels,
   Y offset from the top left corner in pixels,
   tile number,
   palette number

  Metasprite ends with X offset 128.

 Put metasprite with H flip as C (Shift+N)

  The same as previous function, but nametable selection is flipped
  horizontally, and |OAM_FLIP_H added to the palette number


Metasprites

 Open metasprite bank

  Opens a set of 256 metasprites from a binary file.

 Save metasprite bank

  Saves a set of 256 metasprites into a binary file.

 Put metasprite as C data to clipboard

  Copy current metasprite into the Windows clipboard as an array
  definition in special format, to be pasted into C code. The
  format is the same as for similar function for the nametable,
  explained above.

 Put bank as C data to clipboard

  Copy all 256 metasprites into the Windows clipboard as two
  arrays in special format. First array contains 256 pointers
  to actual metasprite data, including missing metasprites.
  Second array contain metasprite definitions similar to the
  format explained above, but it contains all non-empty
  metasprites following one by another.


Palettes

 Open palettes

  Open palette file.

 Save palettes

  Save current palette to a file.

 Put code to clipboard

  Copy current palette as an 6502 assembly code, series of LDA/STA opcodes,
  into the Windows clipboard.

 Put ASM data to clipboard

  Copy current palette data as set of db statements into the Windows clipboard.

 Put C data to clipboard

  Copy current palette data as C array into the Windows clipboard.


Import

 BMP file as nametable

  Imports a 256x240 or smaller palettized BMP image with 16 or 256 colors,
  with 256 or less unique tiles, as a nametable. It removes duplicated tiles
  and creates nametable. Only two lower bits of the graphics are used. The
  program also attempts to select similar colors from current NES palette
  (altered with the color emphasis bits) and create attributes map.

 BMP file as tileset

  Imports a 128x128 palettized BMP image as a tileset. Nametable remains
  unchanged.

 NES file

  Import a 8K CHR bank from an iNES ROM image. If there is more than 8K of
  graphics in the selected file, you can select which part of the file
  should be imported.


Export

 Nametable as BMP file

  Export current nametable as a palettized BMP file.

 Tileset as BMP file

  Export current tileset as a palettized BMP file.

 Palette as BMP file

  Export current palette as a 16x1 BMP palettized file

 Put CHR in NES file

  Replace 8K CHR bank in a iNES ROM image. If there is more than 8K of graphics
  in the selected file, you can select which part of the file should be
  replaced.


CHR Editor

 Show CHR editor window. You can use these keys there:

  LMB      - set pixel
  RMB      - set current color to the color of the pixel
  Ctrl+LMB - fill an area with current color

  Cursor   - scroll tile with wrap around
  H        - vertical mirror
  V        - horizontal mirror
  Delete   - clear tile
  L        - rotate 90 degree left
  R	       - rotate 90 degree right

  Ctrl+Z   - undo
  Ctrl+X   - cut tile graphics
  Ctrl+C   - copy tile graphics
  Ctrl+V   - paste tile graphics



Miscellaneous features

You can place nes.pal file (192 bytes) in the program directory, if you want
to tweak the colors of the NES palette.

You can associate the program with any file types that are supported by the
All\Open menu item, and open them by double click.



File types

There are many file types supported by the program.

 *.chr - tile graphics, matches to the hardware format
 *.pal - palette, 16 bytes
 *.nam - nametable, 960 or 1024 bytes, matches to the hardware format
 *.atr - attributes, last 64 bytes of full nametable
 *.rle - RLE-packed nametable
 *.h   - standart C header, text format
 *.nes - iNES ROM, supported for CHR import and export
 *.bmp - standart graphics format, supported for import and export
 *.nss - internal format for saving and restoring sessions



History:

v2.03 03.03.13 - Find/Remove unused tiles functions
v2.01 22.01.13 - Metasprite copy/paste and flip, nametable import from files with arbitrary sizes
v2.0  16.01.13 - Metasprite editor, type in feature, minor improvements
v1.50 28.06.12 - Nametable drawing with current CHR selection, like a brush mode
v1.49 20.03.12 - Tile flip for 90 degree, sessions, minor bugfixes, better docs
v1.48 19.02.12 - BMP import now attempts to import palette and attributes, Save all function
v1.47 21.01.12 - Swap banks feature, minor bugfix for tileset import from BMP
v1.46 20.01.12 - BMP export is now 16-color, BMP as a tileset import is added
v1.45 18.01.12 - CHR import/export for NES files, some other features and minor bugfixes
v1.44 22.12.11 - Optimize didn't worked properly for the second bank
v1.43 04.12.11 - Number of selected tiles now displayed in the info box
v1.42 10.10.11 - Attributes table could be saved in a separate file
v1.41 02.08.11 - Filenames of files opened through Open or double click are put into save dialogs
v1.40 31.07.11 - Correct work with files that contain dots in path
v1.39 30.07.11 - Fixed wrong file extension and CHR size that was suggested sometimes
v1.38 27.07.11 - Put block as a metasprite into clipboard function
v1.37 01.07.11 - Attribute checker
v1.36 26.06.11 - Save and copy nametable to clipboard as C code
v1.35 04.06.11 - Tileset and nametable export as bitmap files
v1.34 16.05.11 - CHR import from NES files, supports 24K and 40K ROMs
v1.32 10.05.11 - Autofix for unsafe color $0d in palettes, color emphasis display
v1.31 18.01.11 - Minor fixes and some small features
v1.3  03.01.11 - Bugfixes, some new features
v1.23 11.12.10 - CHR redundancy optimization
v1.22 10.12.10 - Copy/paste tile groups in tileset
v1.21 09.12.10 - One-step undo
v1.2  08.12.10 - 1K, 2K, and 8K CHR files support, additional features for CHR editor
v1.12 07.12.10 - Copying parts of the nametable in the clipboard as text
v1.11 06.12.10 - Detailed information bar
v1.1  05.12.10 - Minor fixes and improvements, open all, nametable copy/paste functions
v1.0  04.12.10 - Initial release



Mail:	shiru@mail.ru
Web:    http:// shiru.untergrund.net
Donate: http://www.justgiving.com/Shiru