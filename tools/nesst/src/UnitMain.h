//---------------------------------------------------------------------------

#ifndef UnitMainH
#define UnitMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Graphics.hpp>
#include <Dialogs.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormMain : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxPal;
	TPaintBox *PaintBoxPal;
	TSpeedButton *SpeedButtonPal;
	TLabel *Label1;
	TGroupBox *GroupBoxTiles;
	TSpeedButton *SpeedButtonTiles;
	TLabel *Label2;
	TImage *ImageTiles;
	TSpeedButton *SpeedButtonGridAll;
	TLabel *Label3;
	TOpenDialog *OpenDialogChr;
	TSaveDialog *SaveDialogChr;
	TMainMenu *MainMenu1;
	TMenuItem *MPatterns;
	TMenuItem *MCHROpen;
	TMenuItem *MCHRSave;
	TOpenDialog *OpenDialogName;
	TSaveDialog *SaveDialogName;
	TMenuItem *MNameTable;
	TMenuItem *MOpenNameTable;
	TMenuItem *MSaveNameTableOnly;
	TMenuItem *MSaveNameTableAttributes;
	TMenuItem *MPalettes;
	TMenuItem *MOpenPalettes;
	TMenuItem *MSavePalettes;
	TMenuItem *MPutCodeToClipboard;
	TOpenDialog *OpenDialogPal;
	TSaveDialog *SaveDialogPal;
	TMenuItem *MImport;
	TMenuItem *MCHREditor;
	TOpenDialog *OpenDialogImport;
	TMenuItem *PutDataToClipboardASM;
	TMenuItem *MAll;
	TOpenDialog *OpenDialogAll;
	TMenuItem *MPutSelectedBlockToClipboardASM;
	TMenuItem *MCHRClear;
	TMenuItem *N1;
	TSpeedButton *SpeedButtonChrBank1;
	TSpeedButton *SpeedButtonChrBank2;
	TMenuItem *MCHRRemoveDoubles;
	TMenuItem *N2;
	TMenuItem *N3;
	TLabel *Label4;
	TLabel *Label5;
	TLabel *Label8;
	TLabel *Label9;
	TLabel *Label10;
	TSpeedButton *SpeedButtonGridTile;
	TSpeedButton *SpeedButtonGridAtr;
	TSpeedButton *SpeedButtonGridBlock;
	TMenuItem *MCHRSaveSel;
	TSaveDialog *SaveDialogChrSelection;
	TMenuItem *MCHRInterleave;
	TMenuItem *MCHRDeinterleave;
	TMenuItem *MCHRSwapColors;
	TMenuItem *MAddOffset;
	TSpeedButton *SpeedButtonMaskB;
	TSpeedButton *SpeedButtonMaskG;
	TSpeedButton *SpeedButtonMaskR;
	TSpeedButton *SpeedButtonMaskM;
	TMenuItem *MExport;
	TMenuItem *MExportNametableBMP;
	TMenuItem *MExportTilesetBMP;
	TSaveDialog *SaveDialogImage;
	TMenuItem *PutDataToClipboardC;
	TMenuItem *MPutSelectedBlockToClipboardC;
	TMenuItem *N4;
	TSpeedButton *SpeedButtonChecker;
	TLabel *Label11;
	TMenuItem *N5;
	TMenuItem *MPutMetaSpriteToClipboardC;
	TMenuItem *MPutMetaSpriteToClipboardHFlipC;
	TMenuItem *MSaveAttributes;
	TSaveDialog *SaveDialogAttr;
	TMenuItem *MCHRFillNumbers;
	TMenuItem *MCHRFindDoubles;
	TMenuItem *N6;
	TMenuItem *N7;
	TMenuItem *N8;
	TMenuItem *MImportBMPNametable;
	TMenuItem *MImportNES;
	TMenuItem *MExportNES;
	TSaveDialog *SaveDialogExportNES;
	TMenuItem *MImportBMPTileset;
	TMenuItem *MCHRSwapBanks;
	TMenuItem *MOpen;
	TMenuItem *MSave;
	TMenuItem *N9;
	TMenuItem *MSaveSession;
	TMenuItem *MLoadSession;
	TOpenDialog *OpenDialogSession;
	TSaveDialog *SaveDialogSession;
	TMenuItem *MExportPaletteBMP;
	TMenuItem *N10;
	TPageControl *PageControlEditor;
	TTabSheet *TabSheetName;
	TTabSheet *TabSheetSprite;
	TImage *ImageName;
	TGroupBox *GroupBoxStats;
	TLabel *LabelStats;
	TSpeedButton *SpeedButtonTypeIn;
	TLabel *Label6;
	TGroupBox *GroupBoxSpriteList;
	TListBox *ListBoxSpriteList;
	TSpeedButton *SpeedButtonSpriteDel;
	TSpeedButton *SpeedButtonSpriteUp;
	TSpeedButton *SpeedButtonSpriteDown;
	TGroupBox *GroupBoxMetaSprite;
	TImage *ImageMetaSprite;
	TSpeedButton *SpeedButtonPrevMetaSprite;
	TLabel *LabelMetaSprite;
	TSpeedButton *SpeedButtonNextMetaSprite;
	TSpeedButton *SpeedButtonClearMetaSprite;
	TLabel *Label7;
	TSpeedButton *SpeedButtonFrameAll;
	TSpeedButton *SpeedButtonFrameSelected;
	TSpeedButton *SpeedButtonFrameNone;
	TSpeedButton *SpeedButtonSpriteSnap;
	TMenuItem *MMetaSprites;
	TMenuItem *MOpenMetaSpriteBank;
	TMenuItem *MSaveMetaSpriteBank;
	TMenuItem *MMetaSpritePutBankCToClipboard;
	TMenuItem *N11;
	TOpenDialog *OpenDialogMetaSpriteBank;
	TSaveDialog *SaveDialogMetaSpriteBank;
	TMenuItem *MMetaSpritePutCToClipboard;
	TSpeedButton *SpeedButtonSpriteHFlip;
	TSpeedButton *SpeedButtonSpriteVFlip;
	TSpeedButton *SpeedButtonMetaSpriteCopy;
	TSpeedButton *SpeedButtonMetaSpritePaste;
	TSpeedButton *SpeedButtonMetaSpriteHFlip;
	TSpeedButton *SpeedButtonMetaSpriteVFlip;
	TMenuItem *MCHRFindUnused;
	TMenuItem *MCHRRemoveUnused;
	TMenuItem *N12;
	void __fastcall FormPaint(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall PaintBoxPalMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ImageTilesMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonGridAllClick(TObject *Sender);
	void __fastcall MCHROpenClick(TObject *Sender);
	void __fastcall PaintBoxNamePaint(TObject *Sender);
	void __fastcall ImageNameMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ImageNameMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall MOpenNameTableClick(TObject *Sender);
	void __fastcall MSaveNameTableOnlyClick(TObject *Sender);
	void __fastcall MSaveNameTableAttributesClick(TObject *Sender);
	void __fastcall MOpenPalettesClick(TObject *Sender);
	void __fastcall MSavePalettesClick(TObject *Sender);
	void __fastcall MPutCodeToClipboardClick(TObject *Sender);
	void __fastcall PaintBoxPalPaint(TObject *Sender);
	void __fastcall MCHREditorClick(TObject *Sender);
	void __fastcall MCHRSaveClick(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall PutDataToClipboardASMClick(TObject *Sender);
	void __fastcall ImageTilesDblClick(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ImageNameMouseLeave(TObject *Sender);
	void __fastcall ImageTilesMouseLeave(TObject *Sender);
	void __fastcall ImageTilesMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall MPutSelectedBlockToClipboardASMClick(TObject *Sender);
	void __fastcall MCHRClearClick(TObject *Sender);
	void __fastcall SpeedButtonChrBank1Click(TObject *Sender);
	void __fastcall MCHRRemoveDoublesClick(TObject *Sender);
	void __fastcall PaintBoxPalMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall PaintBoxPalMouseLeave(TObject *Sender);
	void __fastcall MCHRSaveSelClick(TObject *Sender);
	void __fastcall MCHRInterleaveClick(TObject *Sender);
	void __fastcall MCHRDeinterleaveClick(TObject *Sender);
	void __fastcall MCHRSwapColorsClick(TObject *Sender);
	void __fastcall MAddOffsetClick(TObject *Sender);
	void __fastcall SpeedButtonMaskBClick(TObject *Sender);
	void __fastcall SpeedButtonMaskGClick(TObject *Sender);
	void __fastcall SpeedButtonMaskRClick(TObject *Sender);
	void __fastcall SpeedButtonMaskMClick(TObject *Sender);
	void __fastcall MExportNametableBMPClick(TObject *Sender);
	void __fastcall MExportTilesetBMPClick(TObject *Sender);
	void __fastcall PutDataToClipboardCClick(TObject *Sender);
	void __fastcall MPutSelectedBlockToClipboardCClick(TObject *Sender);
	void __fastcall SpeedButtonCheckerClick(TObject *Sender);
	void __fastcall MPutMetaSpriteToClipboardCClick(TObject *Sender);
	void __fastcall MPutMetaSpriteToClipboardHFlipCClick(TObject *Sender);
	void __fastcall MSaveAttributesClick(TObject *Sender);
	void __fastcall MCHRFillNumbersClick(TObject *Sender);
	void __fastcall MCHRFindDoublesClick(TObject *Sender);
	void __fastcall MImportBMPNametableClick(TObject *Sender);
	void __fastcall MImportNESClick(TObject *Sender);
	void __fastcall MExportNESClick(TObject *Sender);
	void __fastcall MImportBMPTilesetClick(TObject *Sender);
	void __fastcall MCHRSwapBanksClick(TObject *Sender);
	void __fastcall MOpenClick(TObject *Sender);
	void __fastcall MSaveClick(TObject *Sender);
	void __fastcall MLoadSessionClick(TObject *Sender);
	void __fastcall MSaveSessionClick(TObject *Sender);
	void __fastcall MExportPaletteBMPClick(TObject *Sender);
	void __fastcall SpeedButtonTypeInClick(TObject *Sender);
	void __fastcall FormKeyPress(TObject *Sender, char &Key);
	void __fastcall PageControlEditorChange(TObject *Sender);
	void __fastcall ImageMetaSpriteDragOver(TObject *Sender, TObject *Source,
          int X, int Y, TDragState State, bool &Accept);
	void __fastcall ImageMetaSpriteDragDrop(TObject *Sender, TObject *Source,
          int X, int Y);
	void __fastcall ImageMetaSpriteEndDrag(TObject *Sender, TObject *Target, int X,
          int Y);
	void __fastcall SpeedButtonPrevMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonNextMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonClearMetaSpriteClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteDelClick(TObject *Sender);
	void __fastcall ListBoxSpriteListClick(TObject *Sender);
	void __fastcall SpeedButtonFrameSelectedClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteUpClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteDownClick(TObject *Sender);
	void __fastcall ListBoxSpriteListKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall ImageMetaSpriteMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ImageMetaSpriteMouseLeave(TObject *Sender);
	void __fastcall ImageMetaSpriteMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
	void __fastcall ImageMetaSpriteMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall MOpenMetaSpriteBankClick(TObject *Sender);
	void __fastcall MSaveMetaSpriteBankClick(TObject *Sender);
	void __fastcall MMetaSpritePutBankCToClipboardClick(TObject *Sender);
	void __fastcall MMetaSpritePutCToClipboardClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonSpriteVFlipClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteCopyClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpritePasteClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonMetaSpriteVFlipClick(TObject *Sender);
	void __fastcall MCHRFindUnusedClick(TObject *Sender);
	void __fastcall MCHRRemoveUnusedClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormMain(TComponent* Owner);
	void __fastcall DrawPalettes(void);
	void __fastcall DrawCol(int,int,int,int,bool);
	void __fastcall DrawBGPal(int,int,int);
	void __fastcall DrawSelection(TImage*,TRect);
	void __fastcall DrawTile(TPicture*,int,int,int,int,int,int,bool);
	void __fastcall DrawTileChecker(TPicture*,int,int,int,int,int,int,bool);
	void __fastcall DrawSmallTile16(TPicture*,int,int,int,int,int,int);
	void __fastcall UpdateTiles(bool);
	void __fastcall UpdateNameTable(int,int,bool);
	void __fastcall CopyCHR(bool,bool);
	void __fastcall PasteCHR(void);
	void __fastcall NameTableScrollLeft(bool);
	void __fastcall NameTableScrollRight(bool);
	void __fastcall NameTableScrollUp(bool);
	void __fastcall NameTableScrollDown(bool);
	bool __fastcall OpenCHR(AnsiString);
	void __fastcall InterleaveCHR(bool);
	bool __fastcall OpenNameTable(AnsiString);
	bool __fastcall OpenPalette(AnsiString);
	void __fastcall CopyMap(bool);
	void __fastcall CopyMapCodeASM(void);
	void __fastcall CopyMapCodeC(void);
	void __fastcall PasteMap(void);
	void __fastcall FillMap(void);
	void __fastcall GetSelection(TRect,int&,int&,int&,int&);
	void __fastcall OpenAll(AnsiString);
	void __fastcall UpdateStats(void);
	void __fastcall SetUndo(void);
	void __fastcall Undo(void);
	void __fastcall SetTile(int);
	void __fastcall CopyMetaSpriteCodeC(bool);
	void __fastcall SetBMPPalette(Graphics::TBitmap*);
	void __fastcall LoadSession1x(AnsiString);
	void __fastcall LoadSession(AnsiString);
	void __fastcall SaveSession(AnsiString);
	void __fastcall UpdateRGBM(void);
	bool __fastcall MouseTypeIn(int,int);
	void __fastcall NameTableTypeIn(int);
	void __fastcall UpdateMetaSprite(void);
	void __fastcall DrawSpriteTile(TPicture*,int,int,int,int,TColor);
	void __fastcall MoveSprite(int,int);
	void __fastcall SelectSprite(int);
	void __fastcall SelectTile(int);
	void __fastcall SelectPalette(int);
	bool __fastcall OpenMetaSprites(AnsiString);
	void __fastcall FindDoublesUnused(bool);
	void __fastcall RemoveDoublesUnused(bool);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMain *FormMain;
//---------------------------------------------------------------------------
#endif
