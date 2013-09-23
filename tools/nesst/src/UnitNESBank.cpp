//---------------------------------------------------------------------------

#include <vcl.h>
#include <stdio.h>
#pragma hdrstop

#include "UnitNESBank.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormBank *FormBank;



void __fastcall TFormBank::DrawBank(TPicture* pic,unsigned char* chr)
{
	const int palette[4]={ 0x000000,0x606060,0xc0c0c0,0xffffff };
	unsigned char *dst;
	int i,j,k,pp,col,size;
	int x,y;

	pp=0;

	for(y=0;y<128;y+=8)
	{
		for(x=0;x<128;x+=8)
		{
			for(i=0;i<8;i++)
			{
				dst=(unsigned char*)pic->Bitmap->ScanLine[y+i]+x*3;

				for(j=0;j<8;j++)
				{
					col=palette[(((chr[pp+i]<<j)&128)>>7)|(((chr[pp+i+8]<<j)&128)>>6)];

					*dst++=(col>>16)&0xff;
					*dst++=(col>>8)&0xff;
					*dst++=(col&0xff);
				}
			}

			pp+=16;
		}
	}
}



void __fastcall TFormBank::WrongFile(void)
{
	unsigned char chr[4096];

	memset(chr,0,4096);

	DrawBank(ImageCHR1->Picture,chr);
	DrawBank(ImageCHR2->Picture,chr);

	ImageCHR1->Repaint();
	ImageCHR2->Repaint();

	ButtonOK->Enabled=false;
	UpDownBank->Enabled=false;
	EditBank->Enabled=false;
}



void __fastcall TFormBank::ShowBank(void)
{
	FILE *file;
	unsigned char header[16],chr[8192];
	int mapnum,size;

	file=fopen(FileName.c_str(),"rb");

	if(!file)
	{
		WrongFile();
		return;
	}

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	if(size<16+16384+8192)//file is too small
	{
		fclose(file);
		WrongFile();
		return;
	}

	fread(header,16,1,file);

	if(memcmp(header,"NES",3))//not a NES ROM
	{
		fclose(file);
		WrongFile();
		return;
	}

	mapnum=((header[6]>>4)&0x0f)|(header[7]&0xf0);
	PRG=header[4];
	CHR=header[5];

	LabelInfo->Caption="Mapper "+IntToStr(mapnum)+"  PRG x "+IntToStr(PRG)+"  CHR x "+IntToStr(CHR);

	if(!CHR||Bank>=CHR)
	{
		fclose(file);
		WrongFile();
		return;
	}

	fseek(file,16+PRG*16384+Bank*8192,SEEK_SET);
	fread(chr,8192,1,file);
	fclose(file);

	DrawBank(ImageCHR1->Picture,chr);
	DrawBank(ImageCHR2->Picture,chr+4096);

	ImageCHR1->Repaint();
	ImageCHR2->Repaint();

	UpDownBank->Min=0;
	UpDownBank->Max=CHR-1;
	if(UpDownBank->Position>=UpDownBank->Max) UpDownBank->Position=UpDownBank->Max-1;

	ButtonOK->Enabled=true;
	UpDownBank->Enabled=true;
	EditBank->Enabled=true;
}



//---------------------------------------------------------------------------
__fastcall TFormBank::TFormBank(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormBank::FormCreate(TObject *Sender)
{
	Bank=0;
	OK=false;
	FileName="";
}
//---------------------------------------------------------------------------
void __fastcall TFormBank::ButtonCancelClick(TObject *Sender)
{
	OK=false;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormBank::ButtonOKClick(TObject *Sender)
{
	OK=true;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormBank::FormShow(TObject *Sender)
{
	ShowBank();
}
//---------------------------------------------------------------------------

void __fastcall TFormBank::EditBankChange(TObject *Sender)
{
	EditBank->Text=IntToStr(UpDownBank->Position);
	Bank=UpDownBank->Position;
	ShowBank();
}
//---------------------------------------------------------------------------

