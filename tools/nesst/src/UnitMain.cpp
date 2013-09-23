//---------------------------------------------------------------------------

#include <vcl.h>
#include <stdio.h>
#include <Clipbrd.hpp>
#pragma hdrstop

#include "UnitMain.h"
#include "UnitCHREditor.h"
#include "UnitSwapColors.h"
#include "UnitNametableOffset.h"
#include "UnitNESBank.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMain *FormMain;


#include "palette.h"
#include "smallnums.h"

#define REG_SECTION "Software\\Shiru\\NES Screen Tool\\"

const char regWorkingDirectory[]="WorkDir";
const char colBlack=0x0f;

const char sessionIDStr[8]="NSST2ses";

#define SAVE_NAMETABLE_NAM		1
#define SAVE_NAMETABLE_BIN		2
#define SAVE_NAMETABLE_RLE		3
#define SAVE_NAMETABLE_H		4



int basePalette[64];

float emphasis[8][3]={//from nintech.txt
	{100.0,100.0,100.0},
	{ 74.3, 91.5,123.9},
	{ 88.2,108.6, 79.4},
	{ 65.3, 98.0,101.9},
	{127.7,102.6, 90.5},
	{ 97.9, 90.8,102.3},
	{100.1, 98.7, 74.1},
	{ 75.0, 75.0, 75.0}
};


int outPalette[64];
int ppuMask;

unsigned char bgPal[4][4]={{15,0,16,48},{15,1,33,49},{15,6,22,38},{15,9,25,41}};

int bgPalCur;
int palActive;
int tileActive;
int bankActive;
int nameXC;
int nameYC;
int tileXC;
int tileYC;
int palHover;
int palColHover;
int colHover;

unsigned char chr[8192];
unsigned char chrCopy[4096];
unsigned char nameTable[960];
unsigned char attrTable[64];

TRect nameSelection;
int nameCopyWidth;
int nameCopyHeight;

unsigned char nameCopy[960];
unsigned char attrCopy[960];

unsigned char undoChr[8192];
unsigned char undoNameTable[960];
unsigned char undoAttrTable[64];

TRect chrSelection;
int chrCopyWidth;
int chrCopyHeight;
bool chrCopyRect;
bool chrSelected[256];
bool chrSelectRect;

unsigned char metaSprites[256*64*4];

int spriteActive;
int metaSpriteActive;

int spriteGridX;
int spriteGridY;

int spriteHover;
int spriteDrag;
int spriteDragX;
int spriteDragY;

unsigned char metaSpriteCopy[64*4];

#define OAM_FLIP_H	64
#define OAM_FLIP_V	128



AnsiString reg_load_str(const char *name,AnsiString def)
{
	HKEY key;
	DWORD type,size;
	char *str;
	AnsiString ret;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_SECTION,0,KEY_READ,&key)!=ERROR_SUCCESS) return def;

	type=REG_SZ;
	size=1024;
	str=(char*)malloc(size);
	if(RegQueryValueEx(key,name,NULL,&type,(unsigned char*)str,&size)!=ERROR_SUCCESS) ret=def; else ret=str;
	free(str);
	RegCloseKey(key);

	return ret;
}



void reg_save_str(const char *name,AnsiString str)
{
	HKEY key;
	DWORD disp;

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,REG_SECTION,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&disp)!=ERROR_SUCCESS) return;

	RegSetValueEx(key,name,0,REG_SZ,str.c_str(),strlen(str.c_str())+1);
	RegCloseKey(key);
}



int get_file_size(FILE *file)
{
	int size;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	return size;
}



int read_dword(unsigned char *data)
{
	return data[0]+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}



int attr_get(int x,int y)
{
	int pal;

	pal=attrTable[y/4*8+x/4];
	if(x&2) pal>>=2;
	if(y&2) pal>>=4;

	return pal&3;
}



void attr_set(int x,int y,int pal)
{
	int pp,mask;

	pp=y/4*8+x/4;
	mask=3;
	pal=pal&3;

	if(x&2)
	{
		pal<<=2;
		mask<<=2;
	}
	if(y&2)
	{
		pal<<=4;
		mask<<=4;
	}

	attrTable[pp]=(attrTable[pp]&(mask^255))|pal;
}



void save_data(const char *name,unsigned char *src,int size,int type)
{
	char arrname[256],temp[256];
	FILE *file;
	int i,min,tag,pp,len,sym;
	unsigned char *dst;
	int stat[256];
	bool rle;

	rle=(type==SAVE_NAMETABLE_RLE||type==SAVE_NAMETABLE_H)?true:false;

	if(rle)
	{
		dst=(unsigned char*)malloc(size*2);

		for(i=0;i<256;i++) stat[i]=0;
		for(i=0;i<size;i++) stat[src[i]]++;

		min=256;
		tag=255;

		for(i=0;i<256;i++)
		{
			if(stat[i]<min)
			{
				min=stat[i];
				tag=i;
			}
		}

		//tag=255;
		pp=0;
		dst[pp++]=tag;
		len=0;
		sym=-1;

		for(i=0;i<size;i++)
		{
			if(src[i]!=sym||len==255||i==size-1)
			{
				if(src[i]==sym&&i==size-1) len++;
				if(len) dst[pp++]=sym;
				if(len>1)
				{
					if(len==2)
					{
						dst[pp++]=sym;
					}
					else
					{
						dst[pp++]=tag;
						dst[pp++]=len-1;
					}
				}
				sym=src[i];
				len=1;
			}
			else
			{
				len++;
			}
		}

		dst[pp++]=tag;
		dst[pp++]=0;
		size=pp;
	}
	else
	{
		dst=src;
	}

	if(type!=SAVE_NAMETABLE_H)//binary
	{
		file=fopen(name,"wb");

		if(file)
		{
			fwrite(dst,size,1,file);
			fclose(file);
		}
	}
	else//text
	{
		file=fopen(name,"wt");

		if(file)
		{
			i=strlen(name);

			while(i)
			{
				if(name[i]=='\\'||name[i]=='/')
				{
					i++;
					break;
				}
				i--;
			}

			strcpy(arrname,&name[i]);

			for(i=strlen(arrname)-1;i>=0;i--)
			{
				if(arrname[i]=='.')
				{
					arrname[i]=0;
					break;
				}
			}

			if(arrname[0]!='_'&&!(arrname[0]>='a'&&arrname[0]<='z')&&!(arrname[0]>='A'&&arrname[0]<='Z'))
			{
				strcpy(temp,arrname);
				strcpy(arrname,"n");
				strcat(arrname,temp);
			}

			fprintf(file,"const unsigned char %s[%i]={\n",arrname,size);

			for(i=0;i<size;i++)
			{
				fprintf(file,"0x%2.2x",dst[i]);
				if(i<size-1) fprintf(file,",");
				if((i&15)==15||i==(size-1)) fprintf(file,"\n");
			}

			fprintf(file,"};\n");
			fclose(file);
		}
	}

	if(rle) free(dst);
}



void mem_exchange(unsigned char *src,unsigned char *dst,int len)
{
	int temp;

	while(len)
	{
		temp=*src;
		*src++=*dst;
		*dst++=temp;
		len--;
	}
}



void pal_validate(void)
{
	int i,j;

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			switch(bgPal[i][j])
			{
			case 0x0d:
			case 0x1d:
			case 0x0e:
			case 0x1e:
			case 0x2e:
			case 0x3e:
			case 0x1f:
			case 0x2f:
			case 0x3f:
				bgPal[i][j]=colBlack;
				break;
			}
		}
	}
}



void palette_calc(void)
{
	int i;
	float r,g,b;

	if(!(ppuMask&0x01))
	{
		for(i=0;i<64;i++)
		{
			r=((float)((basePalette[i]>>16)&0xff))/255.0;
			g=((float)((basePalette[i]>>8)&0xff))/255.0;
			b=((float)(basePalette[i]&0xff))/255.0;
			r=r*emphasis[ppuMask>>5][0]/100.0;
			g=g*emphasis[ppuMask>>5][1]/100.0;
			b=b*emphasis[ppuMask>>5][2]/100.0;
			if(r>1.0) r=1.0;
			if(g>1.0) g=1.0;
			if(b>1.0) b=1.0;
			outPalette[i]=(((int)(255.0*r))<<16)|(((int)(255.0*g))<<8)|((int)(255.0*b));
		}
	}
	else
	{
		for(i=0;i<64;i++)
		{
			outPalette[i]=basePalette[i&0xf0];
		}
	}
}



AnsiString RemoveExt(AnsiString name)
{
	return ChangeFileExt(name,"");
}



AnsiString GetExt(AnsiString name)
{
	name=ExtractFileName(name);

	return name.SubString(name.LastDelimiter(".")+1,name.Length()-name.LastDelimiter(".")).LowerCase();
}



void __fastcall TFormMain::DrawCol(int x,int y,int size,int c,bool sel)
{
	TRect r;

	r.left  =x;
	r.top   =y;
	r.right =x+size;
	r.Bottom=y+size;

	PaintBoxPal->Canvas->Brush->Color=TColor(outPalette[c]);
	PaintBoxPal->Canvas->FillRect(r);

	if(sel)
	{
		PaintBoxPal->Canvas->Pen->Color=TColor(0xffffff);
		PaintBoxPal->Canvas->Rectangle(r);
		PaintBoxPal->Canvas->Pen->Color=TColor(0);

		r.left  +=1;
		r.top   +=1;
		r.right -=1;
		r.bottom-=1;

		PaintBoxPal->Canvas->Rectangle(r);
	}
}



void __fastcall TFormMain::DrawBGPal(int x,int y,int pal)
{
	DrawCol(x   ,y,20,bgPal[pal][0],pal==palActive&&bgPalCur==0?true:false);
	DrawCol(x+20,y,20,bgPal[pal][1],pal==palActive&&bgPalCur==1?true:false);
	DrawCol(x+40,y,20,bgPal[pal][2],pal==palActive&&bgPalCur==2?true:false);
	DrawCol(x+60,y,20,bgPal[pal][3],pal==palActive&&bgPalCur==3?true:false);
}



void __fastcall TFormMain::DrawPalettes(void)
{
	int i,j,x,y,pp,col;

	if(!Visible) return;

	DrawBGPal( 16, 0,0);
	DrawBGPal(128, 0,1);
	DrawBGPal( 16,32,2);
	DrawBGPal(128,32,3);

	y=64;
	pp=0;

	col=bgPal[palActive][bgPalCur];

	if(col==0x0f) col=0x0d;

	for(i=0;i<4;i++)
	{
		x=0;

		for(j=0;j<14;j++)
		{
			DrawCol(x,y,16,pp,pp==col);

			pp++;
			x+=16;
		}

		pp+=2;
		y+=16;
	}
}



void __fastcall TFormMain::DrawTile(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty,bool sel)
{
	int j,k,pp,col,r,g,b;
	unsigned char *d1,*d2;
	int hgrid,vgrid,hcol,vcol;

	pp=tile*16+bankActive;

	if(chrSelectRect) sel=false;

	for(j=0;j<8;++j)
	{
		d1=(unsigned char*)pic->Bitmap->ScanLine[y+j*2  ]+x*3;
		d2=(unsigned char*)pic->Bitmap->ScanLine[y+j*2+1]+x*3;

		for(k=0;k<8;++k)
		{
			col=(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
			col=outPalette[bgPal[pal][col]];

			r=(col>>16)&0xff;
			g=(col>>8)&0xff;
			b= col&0xff;

			if(sel)
			{
				r-=64;
				if(r<0) r=0;
				g+=64;
				if(g>255) g=255;
				b-=64;
				if(b<0) b=0;
			}
			*d1++=r;
			*d1++=g;
			*d1++=b;
			*d1++=r;
			*d1++=g;
			*d1++=b;
			*d2++=r;
			*d2++=g;
			*d2++=b;
			*d2++=r;
			*d2++=g;
			*d2++=b;
		}
	}

	hgrid=0;
	vgrid=0;
	hcol=64;
	vcol=64;

	if(tx<0&&ty<0)
	{
		if(SpeedButtonGridTile->Down||SpeedButtonGridAtr->Down||SpeedButtonGridBlock->Down)
		{
			hgrid=2;
			vgrid=2;
		}
	}
	else
	{
		if(SpeedButtonGridTile->Down)
		{
			hgrid=2;
			vgrid=2;
		}
		if(SpeedButtonGridAtr->Down)
		{
			if(!(ty&1))
			{
				hcol=96;
				hgrid=1;
			}
			if(!(tx&1))
			{
				vcol=96;
				vgrid=1;
			}
		}
		if(SpeedButtonGridBlock->Down)
		{
			if(!(ty&3))
			{
				hcol=128;
				hgrid=1;
			}
			if(!(tx&3))
			{
				vcol=128;
				vgrid=1;
			}
		}
	}

	if(hgrid)
	{
		d1=(unsigned char*)pic->Bitmap->ScanLine[y]+x*3;

		for(j=0;j<16;j+=hgrid)
		{
			*d1+++=hcol;
			*d1+++=hcol;
			*d1+++=hcol;

			if(hgrid>1) d1+=(hgrid-1)*3;
		}
	}

	if(vgrid)
	{
		for(j=1;j<16;j+=vgrid)
		{
			d1=(unsigned char*)pic->Bitmap->ScanLine[y+j]+x*3;

			*d1+++=vcol;
			*d1+++=vcol;
			*d1+++=vcol;
		}
		*d1=*d1;//to prevent warning
	}
}



void __fastcall TFormMain::DrawSpriteTile(TPicture *pic,int x,int y,int tile,int attr,TColor frame)
{
	int tx,ty,j,k,pp,col,r,g,b;
	unsigned char *d1,*d2;
	unsigned char chrt[8][8];

	pp=tile*16+bankActive;

	for(j=0;j<8;j++)
	{
		for(k=0;k<8;k++)
		{
			if(attr&128) ty=7-j; else ty=j;//vflip
			if(attr&64 ) tx=7-k; else tx=k;//hflip

			chrt[ty][tx]=(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
		}
	}

	for(j=0;j<8;j++)
	{
		d1=(unsigned char*)pic->Bitmap->ScanLine[y+j*2  ]+x*3;
		d2=(unsigned char*)pic->Bitmap->ScanLine[y+j*2+1]+x*3;

		for(k=0;k<8;k++)
		{
			col=chrt[j][k];

			if(col)
			{
				col=outPalette[bgPal[attr&3][col]];

				r=(col>>16)&0xff;
				g=(col>>8)&0xff;
				b= col&0xff;

				*d1++=r;
				*d1++=g;
				*d1++=b;
				*d1++=r;
				*d1++=g;
				*d1++=b;
				*d2++=r;
				*d2++=g;
				*d2++=b;
				*d2++=r;
				*d2++=g;
				*d2++=b;
			}
			else
			{
				d1+=6;
				d2+=6;
			}
		}
	}

	if(frame!=clBlack)
	{
		pic->Bitmap->Canvas->Brush->Style=bsClear;
		pic->Bitmap->Canvas->Pen->Color=frame;
		pic->Bitmap->Canvas->Rectangle(x,y,x+16,y+16);
	}
}



void __fastcall TFormMain::DrawTileChecker(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty,bool sel)
{
	const unsigned char checker[16]={
		0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
		0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff
	};
	int j,k,col,r,g,b;
	unsigned char *d1,*d2;
	int hgrid,vgrid,hcol,vcol;

	if(chrSelectRect) sel=false;

	for(j=0;j<8;j++)
	{
		d1=(unsigned char*)pic->Bitmap->ScanLine[y+j*2]+x*3;
		d2=(unsigned char*)pic->Bitmap->ScanLine[y+j*2+1]+x*3;
		for(k=0;k<8;k++)
		{
			col=(((checker[j]<<k)&128)>>7)|(((checker[j+8]<<k)&128)>>6);
			col=outPalette[bgPal[pal][col]];
			r=(col>>16)&0xff;
			g=(col>>8)&0xff;
			b=col&0xff;
			if(sel)
			{
				r-=64;
				if(r<0) r=0;
				g+=64;
				if(g>255) g=255;
				b-=64;
				if(b<0) b=0;
			}
			*d1++=r;
			*d1++=g;
			*d1++=b;
			*d1++=r;
			*d1++=g;
			*d1++=b;
			*d2++=r;
			*d2++=g;
			*d2++=b;
			*d2++=r;
			*d2++=g;
			*d2++=b;
		}
	}

	hgrid=0;
	vgrid=0;
	hcol=64;
	vcol=64;

	if(tx<0&&ty<0)
	{
		if(SpeedButtonGridTile->Down||SpeedButtonGridAtr->Down||SpeedButtonGridBlock->Down)
		{
			hgrid=2;
			vgrid=2;
		}
	}
	else
	{
		if(SpeedButtonGridTile->Down)
		{
			hgrid=2;
			vgrid=2;
		}
		if(SpeedButtonGridAtr->Down)
		{
			if(!(ty&1))
			{
				hcol=96;
				hgrid=1;
			}
			if(!(tx&1))
			{
				vcol=96;
				vgrid=1;
			}
		}
		if(SpeedButtonGridBlock->Down)
		{
			if(!(ty&3))
			{
				hcol=128;
				hgrid=1;
			}
			if(!(tx&3))
			{
				vcol=128;
				vgrid=1;
			}
		}
	}

	if(hgrid)
	{
		d1=(unsigned char*)pic->Bitmap->ScanLine[y]+x*3;
		for(j=0;j<16;j+=hgrid)
		{
			*d1+++=hcol;
			*d1+++=hcol;
			*d1+++=hcol;
			if(hgrid>1) d1+=(hgrid-1)*3;
		}
	}

	if(vgrid)
	{
		for(j=1;j<16;j+=vgrid)
		{
			d1=(unsigned char*)pic->Bitmap->ScanLine[y+j]+x*3;
			*d1+++=vcol;
			*d1+++=vcol;
			*d1+++=vcol;
		}
		*d1=*d1;//to prevent warning
	}
}



void __fastcall TFormMain::DrawSmallTile16(TPicture *pic,int x,int y,int tile,int pal,int tx,int ty)
{
	int j,k,pp;;
	unsigned char *dst;
	int col1,col2;

	pp=tile*16+bankActive;

	for(j=0;j<8;j++)
	{
		dst=(unsigned char*)pic->Bitmap->ScanLine[y+j]+x/2;

		for(k=0;k<8;k+=2)
		{
			col1=(pal<<2)|(((chr[pp+j]<<k)&128)>>7)|(((chr[pp+j+8]<<k)&128)>>6);
			col2=(pal<<2)|(((chr[pp+j]<<(k+1))&128)>>7)|(((chr[pp+j+8]<<(k+1))&128)>>6);
			*dst++=col2|(col1<<4);
		}
	}
}



void __fastcall TFormMain::DrawSelection(TImage *image,TRect rect)
{
	TRect r;

	if(rect.left>=0&&rect.top>=0)
	{
		r.left=rect.left*16;
		r.top=rect.top*16;
		r.right=rect.right*16;
		r.bottom=rect.bottom*16;

		image->Canvas->Brush->Style=bsClear;
		image->Canvas->Pen->Color=TColor(0xffffff);
		image->Canvas->Rectangle(r);
		r.left+=1;
		r.top+=1;
		r.right-=1;
		r.bottom-=1;
		image->Canvas->Pen->Color=TColor(0x000000);
		image->Canvas->Rectangle(r);
		r.left-=2;
		r.top-=2;
		r.right+=2;
		r.bottom+=2;
		image->Canvas->Pen->Color=TColor(0x000000);
		image->Canvas->Rectangle(r);
	}
}



void __fastcall TFormMain::UpdateTiles(bool updchr)
{
	int i,x,y;

	if(!Visible) return;

	x=0;
	y=0;

	for(i=0;i<256;i++)
	{
		DrawTile(ImageTiles->Picture,x,y,i,palActive,-1,-1,chrSelected[i]);
		x+=16;
		if(x>=256)
		{
			x=0;
			y+=16;
		}
	}

	if(chrSelectRect) DrawSelection(ImageTiles,chrSelection);

	ImageTiles->Repaint();

	if(updchr) FormCHREditor->PaintBoxChr->Repaint();
}



void __fastcall TFormMain::UpdateNameTable(int tx,int ty,bool repaint)
{
	int i,j,x,y;

	if(!Visible) return;

	if(tx<0||ty<0)
	{
		y=0;

		for(i=0;i<30;i++)
		{
			x=0;
			for(j=0;j<32;j++)
			{
				if(SpeedButtonChecker->Down)
				{
					DrawTileChecker(ImageName->Picture,x,y,nameTable[i*32+j],attr_get(j,i),j,i,false);
				}
				else
				{
					DrawTile(ImageName->Picture,x,y,nameTable[i*32+j],attr_get(j,i),j,i,false);
				}

				x+=16;
			}
			y+=16;
		}
	}
	else
	{
		tx&=~1;
		ty&=~1;

		y=ty*16;

		for(i=0;i<2;i++)
		{
			x=tx*16;

			for(j=0;j<2;j++)
			{
				if(SpeedButtonChecker->Down)
				{
					DrawTileChecker(ImageName->Picture,x,y,nameTable[(ty+i)*32+tx+j],attr_get(tx+j,ty+i),tx+j,ty+i,false);
				}
				else
				{
					DrawTile(ImageName->Picture,x,y,nameTable[(ty+i)*32+tx+j],attr_get(tx+j,ty+i),tx+j,ty+i,false);
				}

				x+=16;
			}

			y+=16;
		}
	}

	DrawSelection(ImageName,nameSelection);

	if(repaint) ImageName->Repaint();
}



void __fastcall TFormMain::CopyCHR(bool copy,bool cut)
{
	int i,j,k,pp,ps,x,y,w,h;

	if(chrSelectRect)
	{
		GetSelection(chrSelection,x,y,w,h);
		if(w&&h&&cut) SetUndo();

		pp=0;

		for(i=0;i<h;i++)
		{
			for(j=0;j<w;j++)
			{
				for(k=0;k<16;k++)
				{
					ps=bankActive+(x+j)*16+(y+i)*256+k;
					if(copy) chrCopy[pp++]=chr[ps];
					if(cut) chr[ps]=0;
				}
			}
		}

		if(copy)
		{
			chrCopyWidth=w;
			chrCopyHeight=h;
			chrCopyRect=true;
		}
	}
	else
	{
		if(cut) SetUndo();

		pp=0;
		w=0;
		h=-1;

		for(i=0;i<256;i++)
		{
			if(chrSelected[i])
			{
				for(j=0;j<16;j++)
				{
					ps=bankActive+i*16+j;
					if(copy) chrCopy[pp++]=chr[ps];
					if(cut) chr[ps]=0;
				}
				w++;
			}
		}

		if(copy)
		{
			chrCopyWidth=w;
			chrCopyHeight=h;
			chrCopyRect=false;
		}
	}

	if(cut)
	{
		UpdateTiles(true);
		UpdateMetaSprite();

		UpdateNameTable(-1,-1,true);
	}
}



void __fastcall TFormMain::PasteCHR(void)
{
	int i,j,k,pp,pd,x,y,w,h;

	if(chrCopyRect)
	{
		if(chrCopyWidth<1||chrCopyHeight<1) return;

		GetSelection(chrSelection,x,y,w,h);
		SetUndo();

		pp=0;

		for(i=0;i<chrCopyHeight;i++)
		{
			for(j=0;j<chrCopyWidth;j++)
			{
				if(x+j<16&&y+i<16)
				{
					for(k=0;k<16;k++)
					{
						pd=bankActive+(x+j)*16+(y+i)*256+k;
						chr[pd]=chrCopy[pp+k];
					}
				}
				pp+=16;
			}
		}
	}
	else
	{
		if(chrCopyWidth<1) return;

		pp=0;
		pd=tileActive*16;

		for(i=0;i<chrCopyWidth;i++)
		{
			pd=bankActive+(pd&0x0fff);
			for(j=0;j<16;j++)
			{
				chr[pd++]=chrCopy[pp++];
			}
		}
	}

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



void __fastcall TFormMain::NameTableScrollLeft(bool all)
{
	unsigned char temp[32];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<30;i++) temp[i]=nameTable[i*32];

		for(i=0;i<31;i++)
		{
			for(j=0;j<30;j++)
			{
				nameTable[j*32+i]=nameTable[j*32+i+1];
			}
		}

		for(i=0;i<30;i++) nameTable[i*32+31]=temp[i];
	}

	if(all)
	{
		for(i=0;i<16;i++) temp[i]=attr_get(0,i*2);

		for(i=0;i<15;i++)
		{
			for(j=0;j<16;j++)
			{
				attr_set(i*2,j*2,attr_get(i*2+2,j*2));
			}
		}

		for(i=0;i<16;i++) attr_set(30,i*2,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollRight(bool all)
{
	unsigned char temp[32];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<30;i++) temp[i]=nameTable[i*32+31];

		for(i=31;i>0;i--)
		{
			for(j=0;j<30;j++)
			{
				nameTable[j*32+i]=nameTable[j*32+i-1];
			}
		}

		for(i=0;i<30;i++) nameTable[i*32]=temp[i];
	}

	if(all)
	{
		for(i=0;i<16;i++) temp[i]=attr_get(30,i*2);

		for(i=15;i>0;i--)
		{
			for(j=0;j<16;j++)
			{
				attr_set(i*2,j*2,attr_get(i*2-2,j*2));
			}
		}

		for(i=0;i<16;i++) attr_set(0,i*2,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollUp(bool all)
{
	unsigned char temp[32];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<32;i++) temp[i]=nameTable[i];

		for(i=0;i<29;i++)
		{
			for(j=0;j<32;j++)
			{
				nameTable[i*32+j]=nameTable[(i+1)*32+j];
			}
		}

		for(i=0;i<32;i++) nameTable[960-32+i]=temp[i];
	}

	if(all)
	{
		for(i=0;i<16;i++) temp[i]=attr_get(i*2,0);

		for(i=0;i<14;i++)
		{
			for(j=0;j<16;j++)
			{
				attr_set(j*2,i*2,attr_get(j*2,i*2+2));
			}
		}

		for(i=0;i<16;i++) attr_set(i*2,28,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::NameTableScrollDown(bool all)
{
	unsigned char temp[32];
	int i,j,k;

	for(k=0;k<(all?2:1);k++)
	{
		for(i=0;i<32;i++) temp[i]=nameTable[960-32+i];

		for(i=29;i>0;i--)
		{
			for(j=0;j<32;j++)
			{
				nameTable[i*32+j]=nameTable[(i-1)*32+j];
			}
		}

		for(i=0;i<32;i++) nameTable[i]=temp[i];
	}

	if(all)
	{
		for(i=0;i<16;i++) temp[i]=attr_get(i*2,28);

		for(i=14;i>0;i--)
		{
			for(j=0;j<16;j++)
			{
				attr_set(j*2,i*2,attr_get(j*2,i*2-2));
			}
		}

		for(i=0;i<16;i++) attr_set(i*2,0,temp[i]);
	}

	UpdateNameTable(-1,-1,true);
}



bool __fastcall TFormMain::OpenCHR(AnsiString name)
{
	unsigned char buf[4096];
	FILE *file;
	int i,pp,size,type;

	file=fopen(name.c_str(),"rb");

	type=-1;

	if(file)
	{
		size=get_file_size(file);

		switch(size)
		{
		case 8192:
			fread(chr,8192,1,file);
			type=4;
			break;

		case 4096:
			fread(chr+bankActive,4096,1,file);
			type=3;
			break;

		default:
			if(size<4096&&!(size&15))
			{
				fread(buf,size,1,file);

				pp=tileActive*16;

				for(i=0;i<size;i++)
				{
					chr[bankActive+pp++]=buf[i];
					if(pp>=4096) pp=0;
				}

				if(size==2048) type=2;
				if(size==1024) type=1;
			}
			else
			{
				Application->MessageBox("Wrong file size","Error",MB_OK);
				fclose(file);
				return false;
			}
		}
	}

	fclose(file);

	if(type<0) return false;

	SaveDialogChr->FilterIndex=type;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();

	return true;
}



void __fastcall TFormMain::InterleaveCHR(bool dir)
{
	unsigned char buf[4096];
	int table[256];
	int i,j,pp,pd,num;

	SetUndo();

	for(i=0;i<16;i++)
	{
		num=(i/2)*32+(i&1);

		for(j=0;j<16;j++)
		{
			if(dir) table[i*16+j]=num; else table[num]=i*16+j;
			num+=2;
		}
	}

	pp=bankActive;

	for(i=0;i<256;i++)
	{
		pd=table[i]*16;

		for(j=0;j<16;j++)
		{
			buf[pd++]=chr[pp++];
		}
	}

	memcpy(chr+bankActive,buf,4096);

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



bool __fastcall TFormMain::OpenNameTable(AnsiString name)
{
	FILE *file;
	int size;
	unsigned char src[2048],dst[1024];
	int i,tag,pp,pd;
	AnsiString ext;

	ext=GetExt(name);

	file=fopen(name.c_str(),"rb");

	if(file)
	{
		if(ext!="rle")
		{
			size=get_file_size(file);

			if(size==960||size==1024)
			{
				fread(nameTable,960,1,file);

				if(size==1024) fread(attrTable,64,1,file);

				fclose(file);

				UpdateNameTable(-1,-1,true);

				return true;
			}
			else
			{
				Application->MessageBox("nameTable should be 960 or 1024 bytes long","Error",MB_OK);
			}
		}
		else
		{
			size=get_file_size(file);

			if(size<2048)
			{
				fread(src,size,1,file);
				fclose(file);

				tag=src[0];
				pp=1;
				pd=0;

				while(pp<size)
				{
					if(src[pp]==tag)
					{
						pp++;
						if(src[pp]==0) break;
						for(i=0;i<src[pp];i++) dst[pd++]=dst[pd-1];
						pp++;
					}
					else
					{
						dst[pd++]=src[pp++];
					}
				}
			}

			if(pd==959||pd==1023) pd++;//for old files saved when RLE packer had bug
			if(pd==960||pd==1024) memcpy(nameTable,dst,960);
			if(pd==1024) memcpy(attrTable,dst+960,64);
			if(pd!=960&&pd!=1024) Application->MessageBox("Can't decode RLE","Error",MB_OK);

			UpdateNameTable(-1,-1,true);

			return true;
		}
	}

	return false;
}



bool __fastcall TFormMain::OpenPalette(AnsiString name)
{
	FILE *file;
	unsigned char pal[16];
	int i;

	file=fopen(name.c_str(),"rb");

	if(file)
	{
		if(get_file_size(file)==16)
		{
			fread(pal,16,1,file);
			fclose(file);

			for(i=0;i<4;i++)
			{
				bgPal[0][i]=pal[i];
				bgPal[1][i]=pal[i+4];
				bgPal[2][i]=pal[i+8];
				bgPal[3][i]=pal[i+12];
			}

			pal_validate();
			DrawPalettes();
			UpdateTiles(true);
			UpdateNameTable(-1,-1,true);
			UpdateMetaSprite();

			return true;
		}
		else
		{
			Application->MessageBox("Palette file should be 16 bytes long","Error",MB_OK);
		}
	}

	return false;
}



void __fastcall TFormMain::GetSelection(TRect r,int &x,int &y,int &w,int &h)
{
	if(r.left<r.right)
	{
		x=r.left;
		w=r.right-x;
	}
	else
	{
		x=r.right;
		w=r.left-x;
	}
	if(r.top<r.bottom)
	{
		y=r.top;
		h=r.bottom-y;
	}
	else
	{
		y=r.bottom;
		h=r.top-y;
	}
}



void __fastcall TFormMain::CopyMap(bool cut)
{
	int i,j,x,y,w,h,pp;

	GetSelection(nameSelection,x,y,w,h);
	if(w&&h&&cut) SetUndo();

	pp=0;

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			nameCopy[pp]=nameTable[y*32+x+j];
			attrCopy[pp]=attr_get(x+j,y);
			if(cut&&SpeedButtonTiles->Down) nameTable[y*32+x+j]=0;
			pp++;
		}
		y++;
	}

	nameCopyWidth=w;
	nameCopyHeight=h;

	if(cut) UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::CopyMapCodeASM(void)
{
	char str[65536],buf[1024];
	int i,j,x,y,w,h;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		strcpy(str,"");

		for(i=0;i<h;i++)
		{
			strcat(str,"\t.db ");

			for(j=0;j<w;j++)
			{
				sprintf(buf,"$%2.2x%c",nameTable[(y+i)*32+x+j],j<w-1?',':'\n');
				strcat(str,buf);
			}
		}

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::CopyMapCodeC(void)
{
	char str[65536],buf[1024];
	int i,j,x,y,w,h;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		sprintf(str,"const unsigned char nametable[%i*%i]={\n",w,h);

		for(i=0;i<h;i++)
		{
			strcat(str,"\t");
			for(j=0;j<w;j++)
			{
				sprintf(buf,"0x%2.2x",nameTable[(y+i)*32+x+j]);
				strcat(str,buf);
				if(i*w+j<w*h-1) strcat(str,",");
			}
			strcat(str,"\n");
		}

		strcat(str,"};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::CopyMetaSpriteCodeC(bool hflip)
{
	char str[65536],buf[1024];
	int i,j,x,y,w,h;

	if(nameSelection.left>=0&&nameSelection.top>=0)
	{
		GetSelection(nameSelection,x,y,w,h);

		sprintf(str,"const unsigned char metasprite[]={\n");

		if(!hflip)
		{
			for(i=0;i<h;i++)
			{
				for(j=0;j<w;j++)
				{
					sprintf(buf,"%i,%i,0x%2.2x,%i,\n",j*8,i*8,nameTable[(y+i)*32+x+j],attr_get(x+j,y+i));
					strcat(str,buf);
				}
			}
		}
		else
		{
			for(i=0;i<h;i++)
			{
				for(j=0;j<w;j++)
				{
					sprintf(buf,"%i,%i,0x%2.2x,%i|OAM_FLIP_H,\n",j*8,i*8,nameTable[(y+i)*32+x+(w-1-j)],attr_get(x+(w-1-j),y+i));
					strcat(str,buf);
				}
			}
		}

		strcat(str,"128\n};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}



void __fastcall TFormMain::PasteMap(void)
{
	int i,j,x,y,w,h,pp;

	if(nameCopyHeight<1||nameCopyWidth<1) return;

	GetSelection(nameSelection,x,y,w,h);
	if(w&&h) SetUndo();

	pp=0;

	for(i=0;i<nameCopyHeight;i++)
	{
		for(j=0;j<nameCopyWidth;j++)
		{
			if(x+j<32&&y<30)
			{
				if(SpeedButtonTiles->Down) nameTable[y*32+x+j]=nameCopy[pp];
				if(SpeedButtonPal->Down) attr_set(x+j,y,attrCopy[pp]);
			}
			pp++;
		}
		y++;
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::FillMap(void)
{
	int i,j,x,y,w,h;

	GetSelection(nameSelection,x,y,w,h);
	if(w&&h) SetUndo();

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			if(SpeedButtonTiles->Down) nameTable[y*32+x+j]=tileActive;
		}
		y++;
	}

	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::OpenAll(AnsiString name)
{
	name=RemoveExt(name);

	if(OpenCHR(name+".chr"))
	{
		SaveDialogChr->FileName=name;
	}
	else
	{
		if(OpenCHR(name+".bin")) SaveDialogChr->FileName=name;
	}

	if(OpenNameTable(name+".nam"))
	{
		SaveDialogName->FileName=name;
		SaveDialogAttr->FileName=name;
	}
	else
	{
		if(OpenNameTable(name+".rle"))
		{
			SaveDialogName->FileName=name;
		}
	}

	if(OpenPalette(name+".pal"))
	{
		SaveDialogPal->FileName=name;
	}

	if(OpenMetaSprites(name+".msb"))
	{
		SaveDialogMetaSpriteBank->FileName=name;
	}
}



void __fastcall TFormMain::UpdateStats(void)
{
	AnsiString str;
	int i,cnt,tile,off,col,sel;

	str="---";

	if(tileXC>=0&&tileYC>=0)
	{
		tile=tileYC*16+tileXC;
		cnt=0;

		for(i=0;i<960;i++) if(nameTable[i]==tile) cnt++;

		sel=0;

		for(i=0;i<256;i++) if(chrSelected[i]) sel++;

		str="Tile:$"+IntToHex(tile,2)+" ("+IntToStr(cnt)+" entries)";

		if(sel) str+=" "+IntToStr(sel)+" selected";
	}

	if(nameXC>=0&&nameYC>=0)
	{
		off=nameYC*32+nameXC;
		str="X:"+IntToStr(nameXC)+" Y:"+IntToStr(nameYC)+" Off:$"+IntToHex(off,3)+" Tile:$"+IntToHex(nameTable[off],2);
		str+=" AtrX:"+IntToStr(nameXC/2)+" AtrY:"+IntToStr(nameYC/2)+" AtrOff:$"+IntToHex(nameYC/4*8+nameXC/4+960,3)+"."+IntToStr((nameXC&2)+(nameYC&2)*2)+" Atr:$"+IntToHex(attrTable[nameYC/4*8+nameXC/4],2)+" Pal:"+IntToStr(attr_get(nameXC,nameYC));
		if(nameSelection.left>=0) str+=" W:"+IntToStr(nameSelection.right-nameSelection.left)+" H:"+IntToStr(nameSelection.bottom-nameSelection.top);
	}

	if(palHover>=0)
	{
		str="Pal:"+IntToStr(palHover)+" Entry:"+IntToStr(palColHover)+" Adr:"+IntToHex(0x3f00+palHover*4+palColHover,4)+" Color:$"+IntToHex(bgPal[palHover][palColHover],2);
	}

	if(colHover>=0)
	{
		col=colHover;
		if(col==0x0d||col==0x1d) col=0x0f;
		str="Color:$"+IntToHex(col,2);
	}

	if(spriteHover>=0)
	{
		off=metaSpriteActive*64*4+spriteHover*4;
		str="Sprite:"+IntToStr(spriteHover)+" X:"+IntToStr(metaSprites[off+3]-spriteGridX)+" Y:"+IntToStr(metaSprites[off]-spriteGridY)+" Tile:$"+IntToHex(metaSprites[off+1],2)+" Pal:"+IntToStr(metaSprites[off+2]&3);
		if(metaSprites[off+2]&OAM_FLIP_H) str+=" HFlip";
		if(metaSprites[off+2]&OAM_FLIP_V) str+=" VFlip";
	}

	LabelStats->Caption=str;
}



void __fastcall TFormMain::SetUndo(void)
{
	memcpy(undoChr,chr,8192);
	memcpy(undoNameTable,nameTable,960);
	memcpy(undoAttrTable,attrTable,64);
}



void __fastcall TFormMain::Undo(void)
{
	mem_exchange(undoChr,chr,8192);
	mem_exchange(undoNameTable,nameTable,960);
	mem_exchange(undoAttrTable,attrTable,64);

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



void __fastcall TFormMain::SetTile(int tile)
{
	int i;

	tileActive=tile;
	chrSelection.left=tile&15;
	chrSelection.top=tile/16;
	chrSelection.right=chrSelection.left+1;
	chrSelection.bottom=chrSelection.top+1;

	for(i=0;i<256;i++) chrSelected[i]=false;
	chrSelected[tile]=true;
	chrSelectRect=true;

	UpdateTiles(true);
}



void __fastcall TFormMain::SetBMPPalette(Graphics::TBitmap* bmp)
{
	PALETTEENTRY pal[16];
	int i,col;

	for(i=0;i<16;i++)
	{
		col=outPalette[bgPal[i>>2][i&3]];
		pal[i].peBlue =(col>>16)&0xff;
		pal[i].peGreen=(col>>8)&0xff;
		pal[i].peRed  =(col&0xff);
	}

	SetPaletteEntries(bmp->Palette,0,16,pal);
}



void fput_i32(int i,FILE *file)
{
	unsigned char buf[4];

	buf[0]= i&255;
	buf[1]=(i>>8 )&255;
	buf[2]=(i>>16)&255;
	buf[3]=(i>>24)&255;

	fwrite(buf,4,1,file);
}



int fget_i32(FILE *file)
{
	unsigned char buf[4];

	fread(buf,4,1,file);

	return buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24);
}



void fput_bool(bool n,FILE *file)
{
	fputc(n?1:0,file);
}



bool fget_bool(FILE *file)
{
	return fgetc(file)?true:false;
}



void fput_ansistring(AnsiString str,FILE *file)
{
	fput_i32(str.Length(),file);
	fwrite(str.c_str(),str.Length(),1,file);
}



AnsiString fget_ansistring(FILE *file)
{
	AnsiString str;
	char *temp;
	int len;

	len=fget_i32(file);

	temp=(unsigned char*)malloc(len+1);
	fread(temp,len,1,file);
	temp[len]=0;
	str=temp;
	free(temp);

	return str;
}



void __fastcall TFormMain::LoadSession1x(AnsiString filename)
{
	FILE *file;
	unsigned char temp[16];
	int i,j;

	file=fopen(filename.c_str(),"rb");

	if(!file)
	{
		Application->MessageBox("Can't load session","Error",MB_OK);
		return;
	}

	//signature

	fread(temp,sizeof(sessionIDStr),1,file);

	if(memcmp(temp,sessionIDStr,sizeof(sessionIDStr)))
	{
		fclose(file);
		Application->MessageBox("Session data is corrupted","Error",MB_OK);
		return;
	}

	//arrays

	fread(chr      ,sizeof(chr)      ,1,file);
	fread(chrCopy  ,sizeof(chrCopy)  ,1,file);
	fread(nameTable,sizeof(nameTable),1,file);
	fread(attrTable,sizeof(attrTable),1,file);
	fread(nameCopy ,sizeof(nameCopy) ,1,file);
	fread(attrCopy ,sizeof(attrCopy) ,1,file);
	fread(undoChr  ,sizeof(undoChr)  ,1,file);
	fread(undoNameTable,sizeof(undoNameTable),1,file);
	fread(undoAttrTable,sizeof(undoAttrTable),1,file);

	//palette

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			bgPal[i][j]=fget_i32(file);
		}
	}

	//screen buttons state

	SpeedButtonTiles    ->Down=fget_bool(file);
	SpeedButtonChecker  ->Down=fget_bool(file);
	SpeedButtonChrBank1 ->Down=fget_bool(file);
	SpeedButtonChrBank2 ->Down=fget_bool(file);
	SpeedButtonGridAll  ->Down=fget_bool(file);
	SpeedButtonGridTile ->Down=fget_bool(file);
	SpeedButtonGridAtr  ->Down=fget_bool(file);
	SpeedButtonGridBlock->Down=fget_bool(file);
	SpeedButtonPal      ->Down=fget_bool(file);

	//variables

	bgPalCur  =fget_i32(file);
	palActive =fget_i32(file);
	tileActive=fget_i32(file);
	bankActive=fget_i32(file);
	ppuMask   =fget_i32(file);

	nameSelection.Left  =fget_i32(file);
	nameSelection.Right =fget_i32(file);
	nameSelection.Top   =fget_i32(file);
	nameSelection.Bottom=fget_i32(file);
	nameCopyWidth       =fget_i32(file);
	nameCopyHeight      =fget_i32(file);

	chrSelection.Left  =fget_i32(file);
	chrSelection.Right =fget_i32(file);
	chrSelection.Top   =fget_i32(file);
	chrSelection.Bottom=fget_i32(file);
	chrCopyWidth       =fget_i32(file);
	chrCopyHeight      =fget_i32(file);
	chrCopyRect        =fget_bool(file);
	chrSelectRect      =fget_bool(file);

	for(i=0;i<256;i++) chrSelected[i]=fget_bool(file);


	//save dialogs settings

	SaveDialogChr ->FilterIndex=fget_i32(file);
	SaveDialogName->FilterIndex=fget_i32(file);
	SaveDialogAttr->FilterIndex=fget_i32(file);

	SaveDialogChr ->FileName=fget_ansistring(file);
	SaveDialogName->FileName=fget_ansistring(file);
	SaveDialogAttr->FileName=fget_ansistring(file);
	SaveDialogPal ->FileName=fget_ansistring(file);

	fclose(file);

	nameXC=-1;
	nameYC=-1;
	tileXC=-1;
	tileYC=-1;
	palHover=-1;
	palColHover=-1;
	colHover=-1;

	UpdateRGBM();
	palette_calc();
	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
}



void __fastcall TFormMain::LoadSession(AnsiString filename)
{
	FILE *file;
	unsigned char temp[16];
	int i,j;

	file=fopen(filename.c_str(),"rb");

	if(!file)
	{
		Application->MessageBox("Can't load session","Error",MB_OK);
		return;
	}

	//signature

	fread(temp,sizeof(sessionIDStr),1,file);

	if(!memcmp(temp,"NESSTses",8))//load old format
	{
		fclose(file);
		LoadSession1x(filename);
		return;
	}

	if(memcmp(temp,sessionIDStr,sizeof(sessionIDStr)))
	{
		fclose(file);
		Application->MessageBox("Session data is corrupted","Error",MB_OK);
		return;
	}

	//arrays

	fread(chr      ,sizeof(chr)      ,1,file);
	fread(chrCopy  ,sizeof(chrCopy)  ,1,file);
	fread(nameTable,sizeof(nameTable),1,file);
	fread(attrTable,sizeof(attrTable),1,file);
	fread(nameCopy ,sizeof(nameCopy) ,1,file);
	fread(attrCopy ,sizeof(attrCopy) ,1,file);
	fread(undoChr  ,sizeof(undoChr)  ,1,file);
	fread(undoNameTable,sizeof(undoNameTable),1,file);
	fread(undoAttrTable,sizeof(undoAttrTable),1,file);
	fread(metaSprites  ,sizeof(metaSprites)  ,1,file);

	//palette

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			bgPal[i][j]=fget_i32(file);
		}
	}

	//screen buttons state

	SpeedButtonTiles    ->Down=fget_bool(file);
	SpeedButtonChecker  ->Down=fget_bool(file);
	SpeedButtonChrBank1 ->Down=fget_bool(file);
	SpeedButtonChrBank2 ->Down=fget_bool(file);
	SpeedButtonGridAll  ->Down=fget_bool(file);
	SpeedButtonGridTile ->Down=fget_bool(file);
	SpeedButtonGridAtr  ->Down=fget_bool(file);
	SpeedButtonGridBlock->Down=fget_bool(file);
	SpeedButtonPal      ->Down=fget_bool(file);
	SpeedButtonTypeIn   ->Down=fget_bool(file);
	SpeedButtonFrameAll ->Down=fget_bool(file);
	SpeedButtonFrameSelected->Down=fget_bool(file);
	SpeedButtonFrameNone    ->Down=fget_bool(file);
	SpeedButtonSpriteSnap   ->Down=fget_bool(file);

	//variables

	bgPalCur  =fget_i32(file);
	palActive =fget_i32(file);
	tileActive=fget_i32(file);
	bankActive=fget_i32(file);
	ppuMask   =fget_i32(file);

	metaSpriteActive=fget_i32(file);
	spriteActive=fget_i32(file);
	spriteGridX =fget_i32(file);
	spriteGridY =fget_i32(file);

	nameSelection.Left  =fget_i32(file);
	nameSelection.Right =fget_i32(file);
	nameSelection.Top   =fget_i32(file);
	nameSelection.Bottom=fget_i32(file);
	nameCopyWidth       =fget_i32(file);
	nameCopyHeight      =fget_i32(file);

	chrSelection.Left  =fget_i32(file);
	chrSelection.Right =fget_i32(file);
	chrSelection.Top   =fget_i32(file);
	chrSelection.Bottom=fget_i32(file);
	chrCopyWidth       =fget_i32(file);
	chrCopyHeight      =fget_i32(file);
	chrCopyRect        =fget_bool(file);
	chrSelectRect      =fget_bool(file);

	for(i=0;i<256;i++) chrSelected[i]=fget_bool(file);

	//save dialogs settings

	SaveDialogChr ->FilterIndex=fget_i32(file);
	SaveDialogName->FilterIndex=fget_i32(file);
	SaveDialogAttr->FilterIndex=fget_i32(file);

	SaveDialogChr ->FileName=fget_ansistring(file);
	SaveDialogName->FileName=fget_ansistring(file);
	SaveDialogAttr->FileName=fget_ansistring(file);
	SaveDialogPal ->FileName=fget_ansistring(file);
	SaveDialogMetaSpriteBank->FileName=fget_ansistring(file);

	fclose(file);

	nameXC=-1;
	nameYC=-1;
	tileXC=-1;
	tileYC=-1;
	palHover=-1;
	palColHover=-1;
	colHover=-1;

	UpdateRGBM();
	palette_calc();
	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



void __fastcall TFormMain::SaveSession(AnsiString filename)
{
	FILE *file;
	int i,j;

	file=fopen(filename.c_str(),"wb");

	if(!file)
	{
		Application->MessageBox("Can't save session","Error",MB_OK);
		return;
	}

	//signature

	fwrite(sessionIDStr,sizeof(sessionIDStr),1,file);

	//arrays

	fwrite(chr      ,sizeof(chr)      ,1,file);
	fwrite(chrCopy  ,sizeof(chrCopy)  ,1,file);
	fwrite(nameTable,sizeof(nameTable),1,file);
	fwrite(attrTable,sizeof(attrTable),1,file);
	fwrite(nameCopy ,sizeof(nameCopy) ,1,file);
	fwrite(attrCopy ,sizeof(attrCopy) ,1,file);
	fwrite(undoChr  ,sizeof(undoChr)  ,1,file);
	fwrite(undoNameTable,sizeof(undoNameTable),1,file);
	fwrite(undoAttrTable,sizeof(undoAttrTable),1,file);
	fwrite(metaSprites  ,sizeof(metaSprites)  ,1,file);

	//palette

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			fput_i32(bgPal[i][j],file);
		}
	}

	//screen buttons state

	fput_bool(SpeedButtonTiles    ->Down,file);
	fput_bool(SpeedButtonChecker  ->Down,file);
	fput_bool(SpeedButtonChrBank1 ->Down,file);
	fput_bool(SpeedButtonChrBank2 ->Down,file);
	fput_bool(SpeedButtonGridAll  ->Down,file);
	fput_bool(SpeedButtonGridTile ->Down,file);
	fput_bool(SpeedButtonGridAtr  ->Down,file);
	fput_bool(SpeedButtonGridBlock->Down,file);
	fput_bool(SpeedButtonPal      ->Down,file);
	fput_bool(SpeedButtonTypeIn   ->Down,file);
	fput_bool(SpeedButtonFrameAll ->Down,file);
	fput_bool(SpeedButtonFrameSelected->Down,file);
	fput_bool(SpeedButtonFrameNone    ->Down,file);
	fput_bool(SpeedButtonSpriteSnap   ->Down,file);

	//variables

	fput_i32(bgPalCur  ,file);
	fput_i32(palActive ,file);
	fput_i32(tileActive,file);
	fput_i32(bankActive,file);
	fput_i32(ppuMask   ,file);

	fput_i32(metaSpriteActive,file);
	fput_i32(spriteActive,file);
	fput_i32(spriteGridX ,file);
	fput_i32(spriteGridY ,file);

	fput_i32(nameSelection.Left  ,file);
	fput_i32(nameSelection.Right ,file);
	fput_i32(nameSelection.Top   ,file);
	fput_i32(nameSelection.Bottom,file);
	fput_i32(nameCopyWidth ,file);
	fput_i32(nameCopyHeight,file);

	fput_i32(chrSelection.Left  ,file);
	fput_i32(chrSelection.Right ,file);
	fput_i32(chrSelection.Top   ,file);
	fput_i32(chrSelection.Bottom,file);
	fput_i32(chrCopyWidth ,file);
	fput_i32(chrCopyHeight,file);
	fput_bool(chrCopyRect  ,file);
	fput_bool(chrSelectRect,file);

	for(i=0;i<256;i++) fput_bool(chrSelected[i],file);

	//save dialogs settings

	fput_i32(SaveDialogChr ->FilterIndex,file);
	fput_i32(SaveDialogName->FilterIndex,file);
	fput_i32(SaveDialogAttr->FilterIndex,file);

	fput_ansistring(SaveDialogChr ->FileName,file);
	fput_ansistring(SaveDialogName->FileName,file);
	fput_ansistring(SaveDialogAttr->FileName,file);
	fput_ansistring(SaveDialogPal ->FileName,file);
	fput_ansistring(SaveDialogMetaSpriteBank->FileName,file);

	fclose(file);
}



void __fastcall TFormMain::UpdateRGBM(void)
{
	SpeedButtonMaskB->Caption="B"+IntToStr((ppuMask&0x80?1:0));
	SpeedButtonMaskG->Caption="G"+IntToStr((ppuMask&0x40?1:0));
	SpeedButtonMaskR->Caption="R"+IntToStr((ppuMask&0x20?1:0));
	SpeedButtonMaskM->Caption="M"+IntToStr((ppuMask&0x01?1:0));
}



bool __fastcall TFormMain::MouseTypeIn(int X,int Y)
{
	if(SpeedButtonTypeIn->Down)
	{
		nameSelection.left  =X/16;
		nameSelection.top   =Y/16;
		nameSelection.right =nameSelection.left+1;
		nameSelection.bottom=nameSelection.top +1;

		UpdateNameTable(-1,-1,true);
		UpdateStats();

		return true;
	}

	return false;
}



void __fastcall TFormMain::NameTableTypeIn(int tile)
{
	int dx,dy;

	if(nameSelection.left>=0&&nameSelection.right>=0)
	{
		dx=nameSelection.left;
		dy=nameSelection.top;

		SetUndo();

		if(SpeedButtonTiles->Down) nameTable[dy*32+dx]=tile;

		if(SpeedButtonPal->Down) attr_set(dx,dy,palActive);

		++nameSelection.left;

		if(nameSelection.left>=32)
		{
			nameSelection.left=0;
			++nameSelection.top;

			if(nameSelection.top>=28) nameSelection.top=0;
		}

		nameSelection.right =nameSelection.left+1;
		nameSelection.bottom=nameSelection.top+1;

		UpdateNameTable(dx,dy,false);
	}
}



void __fastcall TFormMain::UpdateMetaSprite(void)
{
	int i,j,x,y,tile,attr,pp,hcol,vcol;
	char str[128];
	TColor frame;
	unsigned char *d1;

	//clear

	ImageMetaSprite->Picture->Bitmap->Canvas->Brush->Style=bsSolid;
	ImageMetaSprite->Picture->Bitmap->Canvas->Brush->Color=(TColor)outPalette[bgPal[0][0]];
	ImageMetaSprite->Picture->Bitmap->Canvas->FillRect(TRect(0,0,256,256));

	//draw grid

	for(i=0;i<ImageMetaSprite->Width;i+=16)
	{
		d1=(unsigned char*)ImageMetaSprite->Picture->Bitmap->ScanLine[i];

		for(j=0;j<ImageMetaSprite->Width;j+=2)
		{
			hcol=(spriteGridY*2==i)?128:64;

			*d1+++=hcol;
			*d1+++=hcol;
			*d1+++=hcol;

			d1+=3;
		}

		for(j=1;j<ImageMetaSprite->Height;j+=2)
		{
			d1=(unsigned char*)ImageMetaSprite->Picture->Bitmap->ScanLine[j]+i*3;

			vcol=(spriteGridX*2==i)?128:64;

			*d1+++=vcol;
			*d1+++=vcol;
			*d1+++=vcol;
		}

		*d1=*d1;//to prevent warning
	}

	//draw sprites

	pp=metaSpriteActive*64*4+63*4;

	for(i=63;i>=0;--i)//reverse order to make proper sprites drawing priority
	{
		y   =metaSprites[pp+0];
		tile=metaSprites[pp+1];
		attr=metaSprites[pp+2];
		x   =metaSprites[pp+3];

		if(y<255)
		{
			frame=SpeedButtonFrameAll->Down?clGray:clBlack;

			if(!SpeedButtonFrameNone->Down&&(spriteActive==i)) frame=clWhite;

			DrawSpriteTile(ImageMetaSprite->Picture,x*2,y*2,tile,attr,frame);
		}

		pp-=4;
	}

	ImageMetaSprite->Repaint();

	//update list

	ListBoxSpriteList->Clear();

	pp=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[pp]<255)
		{
			x=metaSprites[pp+3]-spriteGridX;
			y=metaSprites[pp+0]-spriteGridY;

			sprintf(str,"%2.2x: X%c%3.3i Y%c%3.3i Tile $%2.2x Pal %i",i,x<0?'-':' ',abs(x),y<0?'-':' ',abs(y),metaSprites[pp+1],metaSprites[pp+2]&3);

			if(metaSprites[pp+2]&OAM_FLIP_H) strcat(str," H");
			if(metaSprites[pp+2]&OAM_FLIP_V) strcat(str," V");

			ListBoxSpriteList->Items->Add(str);
		}

		pp+=4;
	}

	if(spriteActive<0) spriteActive=0;

	ListBoxSpriteList->ItemIndex=spriteActive;

	LabelMetaSprite->Caption="Metasprite "+IntToStr(metaSpriteActive);
}



void squeeze_sprites(void)
{
	int i,j,k,pp;

	pp=0;

	for(i=0;i<256;++i)
	{
		for(j=0;j<63;++j)
		{
			if(metaSprites[pp+j*4]==255)
			{
				memcpy(&metaSprites[pp+j*4],&metaSprites[pp+j*4+4],64*4-j*4-4);

				for(k=0;k<4;++k) metaSprites[pp+63*4+k]=255;
			}
		}

		pp+=64*4;
	}

	pp=metaSpriteActive*64*4+spriteActive*4;

	while(pp>=4)
	{
		if(metaSprites[pp]==255) --spriteActive; else break;

		pp-=4;
	}
}



void __fastcall TFormMain::MoveSprite(int dx,int dy)
{
	int off;

	off=metaSpriteActive*64*4+spriteActive*4;

	if(SpeedButtonSpriteSnap->Down)
	{
		dx*=8;
		dy*=8;
	}

	if(spriteActive>=0)
	{
		if(metaSprites[off]<255)
		{
			if(SpeedButtonSpriteSnap->Down)
			{
				if(dy!=0)
				{
					if(metaSprites[off+0]&7) metaSprites[off+0]&=~7; else metaSprites[off+0]+=dy;
				}

				if(dx!=0)
				{
					if(metaSprites[off+3]&7) metaSprites[off+3]&=~7; else metaSprites[off+3]+=dx;
				}
			}
			else
			{
				metaSprites[off+0]+=dy;
				metaSprites[off+3]+=dx;
			}



			UpdateMetaSprite();
		}
	}
}




int get_sprite_id(int x,int y)
{
	int i,pp;

	pp=metaSpriteActive*64*4;

	x/=2;
	y/=2;

	for(i=0;i<64;++i)
	{
		if(y>=metaSprites[pp]&&y<metaSprites[pp]+8&&x>=metaSprites[pp+3]&&x<metaSprites[pp+3]+8) return i;

		pp+=4;
	}

	return -1;
}



void sprite_snap(int id)
{
	int x,y,off;

	off=metaSpriteActive*64*4+id*4;

	if(metaSprites[off]<255)
	{
		x=metaSprites[off+3];
		y=metaSprites[off+0];

		if((x&7)<4) x&=~7; else x=(x&~7)+8;
		if((y&7)<4) y&=~7; else y=(y&~7)+8;

		metaSprites[off+3]=x;
		metaSprites[off+0]=y;
	}
}



void __fastcall TFormMain::SelectSprite(int id)
{
	int off;

	spriteActive=id;

	if(spriteActive>=0)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			SelectTile   (metaSprites[off+1]);
			SelectPalette(metaSprites[off+2]&3);

			UpdateTiles(true);
		}
	}

	UpdateMetaSprite();
}



void __fastcall TFormMain::SelectTile(int id)
{
	int i;

	tileActive=id;

	chrSelection.left  =tileActive&15;
	chrSelection.top   =tileActive/16;
	chrSelection.right =chrSelection.left+1;
	chrSelection.bottom=chrSelection.top +1;

	for(i=0;i<256;i++) chrSelected[i]=false;

	chrSelected[tileActive]=true;
	chrSelectRect=true;

	UpdateTiles(true);
}



void __fastcall TFormMain::SelectPalette(int id)
{
	palActive=id;
	DrawPalettes();
}



bool __fastcall TFormMain::OpenMetaSprites(AnsiString name)
{
	FILE *file;
	int size;
	unsigned char data[2];

	file=fopen(name.c_str(),"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	if(size!=256*64*4+2) return false;

	fread(data,2,1,file);
	fread(metaSprites,256*64*4,1,file);
	fclose(file);

	spriteGridX=data[0];
	spriteGridY=data[1];

	UpdateMetaSprite();

	return true;
}



void __fastcall TFormMain::FindDoublesUnused(bool unused)
{
	int i,j,cnt,used;

	cnt=0;

	for(i=0;i<256;++i)
	{
		chrSelected[i]=false;

		if(unused)
		{
			used=0;

			for(j=0;j<960;++j) if(nameTable[j]==i) ++used;

			if(!used)
			{
				chrSelected[i]=true;
				++cnt;
			}
		}
		else
		{
			if(!memcmp(chr+bankActive+(tileActive<<4),chr+bankActive+(i<<4),16))
			{
				chrSelected[i]=true;
				++cnt;
			}
		}
	}

	if(cnt)
	{
		chrSelectRect=false;
		nameSelection.left=-1;
		nameSelection.top=-1;

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateStats();
	}
}



void __fastcall TFormMain::RemoveDoublesUnused(bool unused)
{
	int i,j,k,pp,x,y,w,h,used;
	bool clear[256],skip[256];

	SetUndo();

	GetSelection(chrSelection,x,y,w,h);

	for(i=0;i<256;++i)
	{
		clear[i]=false;
		skip[i]=false;
	}

	if(chrSelectRect)
	{
		if(w>1||h>1)
		{
			for(i=0;i<16;++i)
			{
				for(j=0;j<16;++j)
				{
					skip[i*16+j]=(i>=y&&i<y+h&&j>=x&&j<x+w)?true:false;
				}
			}
		}
	}
	else
	{
		for(i=0;i<256;++i) skip[i]=chrSelected[i];
	}

	for(i=0;i<256;++i)
	{
		if(!clear[i]&&!skip[i])
		{
			if(unused)
			{
				used=0;

				for(j=0;j<960;++j) if(nameTable[j]==i) ++used;

				if(!used) clear[i]=true;
			}
			else
			{
				for(j=i+1;j<256;j++)
				{
					if(!memcmp(&chr[bankActive+i*16],&chr[bankActive+j*16],16)&&!skip[j])
					{
						clear[j]=true;
						for(k=0;k<960;k++) if(nameTable[k]==j) nameTable[k]=i;
					}
				}
			}
		}
	}

	pp=0;

	for(i=0;i<256;++i)
	{
		if(!clear[i]&&!skip[i])
		{
			while(skip[pp/16]) pp+=16;

			for(j=0;j<960;++j) if(nameTable[j]==i) nameTable[j]=pp/16;

			for(j=0;j<16;++j)
			{
				chr[bankActive+pp]=chr[bankActive+i*16+j];
				++pp;
			}
		}
	}

	for(;pp<4096;pp++) if(!skip[pp/16]) chr[bankActive+pp]=0;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}



//---------------------------------------------------------------------------
__fastcall TFormMain::TFormMain(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::FormPaint(TObject *Sender)
{
	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::FormCreate(TObject *Sender)
{
	FILE *file;
	int i,pp;
	unsigned char buf[192];
	AnsiString dir;

	bgPalCur=0;
	palActive=0;
	tileActive=0;
	bankActive=0;
	metaSpriteActive=0;

	spriteGridX=64;
	spriteGridY=64;

	memset(nameTable  ,0  ,sizeof(nameTable));
	memset(attrTable  ,0  ,sizeof(attrTable));
	memset(chr        ,0  ,sizeof(chr));
	memset(chrCopy    ,0  ,sizeof(chrCopy));
	memset(metaSprites,255,sizeof(metaSprites));
	memset(metaSpriteCopy,255,sizeof(metaSpriteCopy));

	dir=ParamStr(0).SubString(0,ParamStr(0).LastDelimiter("\\/"));

	file=fopen((dir+"nes.pal").c_str(),"rb");

	if(file)
	{
		if(get_file_size(file)==192)
		{
			fread(buf,192,1,file);
			fclose(file);

			pp=0;

			for(i=0;i<64;i++)
			{
				basePalette[i]=(buf[pp+2]<<16)|(buf[pp+1]<<8)|buf[pp];
				pp+=3;
			}
		}
	}
	else
	{
		pp=0;

		for(i=0;i<64;i++)
		{
			basePalette[i]=(palette[pp+2]<<16)|(palette[pp+1]<<8)|palette[pp];
			pp+=3;
		}
	}

	ppuMask=0;
	palette_calc();

	nameSelection.left=-1;
	nameSelection.top=-1;
	nameCopyWidth=-1;
	nameCopyHeight=-1;

	chrSelection.left=0;
	chrSelection.right=0;
	chrSelection.right=1;
	chrSelection.bottom=1;
	chrCopyWidth=-1;
	chrCopyHeight=-1;
	chrCopyRect=true;
	for(i=0;i<256;i++) chrSelected[i]=false;
	chrSelected[tileActive]=true;
	chrSelectRect=true;

	tileXC=-1;
	tileYC=-1;
	nameXC=-1;
	nameYC=-1;
	palHover=-1;
	palColHover=-1;
	colHover=-1;
	spriteHover=-1;
	spriteDrag=-1;

	dir=reg_load_str(regWorkingDirectory,"");

	if(ParamStr(1)!="")
	{
		OpenAll(ParamStr(1));
		dir=ParamStr(1);
	}

	SetCurrentDirectory(dir.c_str());

	SetUndo();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,x,y,col,off;

	y=0;
	x=16;

	for(i=0;i<4;i++)
	{
		if(Y>=y&&Y<y+20)
		{
			if(X>=x   &&X<x+20)
			{
				bgPalCur=0;
				palActive=i;
			}
			if(X>=x+20&&X<x+40)
			{
				bgPalCur=1;
				palActive=i;
			}
			if(X>=x+40&&X<x+60)
			{
				bgPalCur=2;
				palActive=i;
			}
			if(X>=x+60&&X<x+80)
			{
				bgPalCur=3;
				palActive=i;
			}
		}
		x+=128-16;
		if(x>128)
		{
			x=16;
			y=32;
		}
	}

	if(PageControlEditor->ActivePage==TabSheetSprite)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			metaSprites[off+2]=(metaSprites[off+2]&~3)+palActive;

			UpdateMetaSprite();
		}
	}

	if(X>=0&&X<14*16&&Y>=64&&Y<64+4*16)
	{
		bgPal[palActive][bgPalCur]=X/16+(Y-64)/16*16;
		col=bgPal[palActive][0];
		bgPal[0][0]=col;
		bgPal[1][0]=col;
		bgPal[2][0]=col;
		bgPal[3][0]=col;
	}

	pal_validate();

	DrawPalettes();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::ImageTilesMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,off;

	if(X>=0&&X<256&&Y>=0&&Y<256)
	{
		if(Shift.Contains(ssCtrl)&&(Shift.Contains(ssLeft)||Shift.Contains(ssRight)))
		{
			chrSelected[Y/16*16+X/16]=Shift.Contains(ssLeft);
			chrSelectRect=false;

			UpdateTiles(true);
		}
		else
		{
			SelectTile(Y/16*16+X/16);
		}

		if(SpeedButtonTypeIn->Down)
		{
			NameTableTypeIn(tileActive);
		}
		else
		{
			nameSelection.left=-1;
			nameSelection.top=-1;
			UpdateTiles(true);
		}

		UpdateNameTable(-1,-1,true);
		UpdateStats();

		if(PageControlEditor->ActivePage==TabSheetSprite)
		{
			if(Shift.Contains(ssRight))
			{
				ImageTiles->BeginDrag(false,-1);
			}

			if(!Shift.Contains(ssRight))
			{
				if(spriteActive>=0)
				{
					off=metaSpriteActive*64*4+spriteActive*4;

					if(metaSprites[off]<255)
					{
						metaSprites[off+1]=tileActive;

						UpdateMetaSprite();
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonGridAllClick(TObject *Sender)
{
	if(((TSpeedButton*)Sender)->GroupIndex==10)
	{
		SpeedButtonGridTile->Down=SpeedButtonGridAll->Down;
		SpeedButtonGridAtr->Down=SpeedButtonGridAll->Down;
		SpeedButtonGridBlock->Down=SpeedButtonGridAll->Down;
	}
	if(!SpeedButtonGridTile->Down||!SpeedButtonGridAtr->Down||!SpeedButtonGridBlock->Down) SpeedButtonGridAll->Down=false;
	if(SpeedButtonGridTile->Down&&SpeedButtonGridAtr->Down&&SpeedButtonGridBlock->Down) SpeedButtonGridAll->Down=true;

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHROpenClick(TObject *Sender)
{
	if(OpenDialogChr->Execute())
	{
		if(OpenCHR(OpenDialogChr->FileName)) SaveDialogChr->FileName=OpenDialogChr->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxNamePaint(TObject *Sender)
{
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseDown(TObject *Sender, TMouseButton Button,
TShiftState Shift, int X, int Y)
{
	int i;

	if(X>=0&&X<512&&Y>=0&&Y<480)
	{
		if(Shift.Contains(ssLeft)) if(MouseTypeIn(X,Y)) return;

		if((Shift.Contains(ssShift)||GetKeyState(VK_CAPITAL))&&Shift.Contains(ssLeft))
		{
			nameSelection.left  =X/16;
			nameSelection.top   =Y/16;
			nameSelection.right =nameSelection.left+1;
			nameSelection.bottom=nameSelection.top +1;

			UpdateNameTable(-1,-1,true);

			chrSelection.right =chrSelection.left+1;
			chrSelection.bottom=chrSelection.top +1;

			for(i=0;i<256;i++) chrSelected[i]=false;

			chrSelected[tileActive]=true;
			chrSelectRect=true;

			UpdateTiles(true);
		}
		else
		{
			if(Shift.Contains(ssLeft)) SetUndo();

			ImageNameMouseMove(Sender,Shift,X,Y);

			if(nameSelection.left>=0||nameSelection.top>=0)
			{
				nameSelection.left=-1;
				nameSelection.top =-1;

				UpdateNameTable(-1,-1,true);
			}
		}
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseMove(TObject *Sender, TShiftState Shift,
int X, int Y)
{
	int i,j,tx,ty,dx,dy;

	if(X>=0&&X<512&&Y>=0&&Y<480)
	{
		tx=X/16;
		ty=Y/16;

		nameXC=tx;
		nameYC=ty;

		if(Shift.Contains(ssLeft)) if(MouseTypeIn(X,Y)) return;

		if(Shift.Contains(ssShift)||GetKeyState(VK_CAPITAL))
		{
			if(Shift.Contains(ssLeft))
			{
				nameSelection.right =tx+(tx>=nameSelection.left?1:0);
				nameSelection.bottom=ty+(ty>=nameSelection.top ?1:0);
			}

			if(Shift.Contains(ssRight))
			{
				nameSelection.left=-1;
				nameSelection.top =-1;
			}

			UpdateNameTable(-1,-1,true);
		}
		else
		{
			if(Shift.Contains(ssLeft))
			{
				for(i=0;i<chrSelection.bottom-chrSelection.top;++i)
				{
					for(j=0;j<chrSelection.right-chrSelection.left;++j)
					{
						dx=tx+j;
						dy=ty+i;

						if(dx>=0&&dx<32&&dy>=0&&dy<30)
						{
							if(SpeedButtonTiles->Down) nameTable[dy*32+dx]=((chrSelection.top+i)<<4)+chrSelection.left+j;

							if(SpeedButtonPal->Down) attr_set(dx,dy,palActive);

							UpdateNameTable(dx,dy,false);
						}
					}
				}

				UpdateNameTable(tx,ty,true);
			}

			if(Shift.Contains(ssRight))
			{
				SelectTile(nameTable[ty*32+tx]);
				SelectPalette(attr_get(tx,ty));
			}
		}
	}
	else
	{
		nameXC=-1;
		nameYC=-1;
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenNameTableClick(TObject *Sender)
{
	if(OpenDialogName->Execute())
	{
		if(OpenNameTable(OpenDialogName->FileName)) SaveDialogName->FileName=OpenDialogName->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveNameTableOnlyClick(TObject *Sender)
{
	AnsiString name;

	SaveDialogName->FileName=RemoveExt(SaveDialogName->FileName);
	SaveDialogName->Title="Save nametable only";

	if(SaveDialogName->Execute())
	{
		name=RemoveExt(SaveDialogName->FileName);

		switch(SaveDialogName->FilterIndex)
		{
		case 2:  name+=".bin"; break;
		case 3:  name+=".rle"; break;
		case 4:  name+=".h"; break;
		default: name+=".nam";
		}

		if(FileExists(name))
		{
			if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return;
		}

		save_data(name.c_str(),nameTable,960,SaveDialogName->FilterIndex);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveNameTableAttributesClick(TObject *Sender)
{
	unsigned char buf[1024];
	AnsiString name;

	SaveDialogName->FileName=RemoveExt(SaveDialogName->FileName);
	SaveDialogName->Title="Save nametable and attributes";

	if(SaveDialogName->Execute())
	{
		name=RemoveExt(SaveDialogName->FileName);

		switch(SaveDialogName->FilterIndex)
		{
		case 2: name+=".bin"; break;
		case 3: name+=".rle"; break;
		case 4: name+=".h"; break;
		default: name+=".nam";
		}

		memcpy(buf,nameTable,960);
		memcpy(buf+960,attrTable,64);

		if(FileExists(name))
		{
			if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return;
		}

		save_data(name.c_str(),nameTable,1024,SaveDialogName->FilterIndex);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenPalettesClick(TObject *Sender)
{
	if(OpenDialogPal->Execute())
	{
		if(OpenPalette(OpenDialogPal->FileName)) SaveDialogPal->FileName=OpenDialogPal->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSavePalettesClick(TObject *Sender)
{
	FILE *file;
	unsigned char pal[16];
	int i;

	if(SaveDialogPal->Execute())
	{
		pal_validate();

		for(i=0;i<4;i++)
		{
			pal[i]=bgPal[0][i];
			pal[i+4]=bgPal[1][i];
			pal[i+8]=bgPal[2][i];
			pal[i+12]=bgPal[3][i];
		}

		file=fopen(SaveDialogPal->FileName.c_str(),"wb");
		if(file)
		{
			fwrite(pal,16,1,file);
			fclose(file);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutCodeToClipboardClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	pal_validate();

	sprintf(str,"\tlda #$3f\n\tsta $2006\n\tlda #$00\n\tsta $2006\n\tldx #$%2.2x\n\tstx $2007\n",bgPal[0][0]);

	for(i=0;i<4;i++)
	{
		if(i) strcat(str,"\tstx $2007\n");
		for(j=1;j<4;j++)
		{
			sprintf(buf,"\tlda #$%2.2x\n\tsta $2007\n",bgPal[i][j]);
			strcat(str,buf);
		}
	}

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalPaint(TObject *Sender)
{
	DrawPalettes();
}
//---------------------------------------------------------------------------



void __fastcall TFormMain::MCHREditorClick(TObject *Sender)
{
	FormCHREditor->Visible^=true;
}
//---------------------------------------------------------------------------



void __fastcall TFormMain::MCHRSaveClick(TObject *Sender)
{
	unsigned char buf[8192];
	FILE *file;
	int i,pp,size;

	if(SaveDialogChr->Execute())
	{
		switch(SaveDialogChr->FilterIndex)
		{
		case 1:  size=1024; break;
		case 2:  size=2048; break;
		case 3:  size=4096; break;
		default: size=8192;
		}

		file=fopen(SaveDialogChr->FileName.c_str(),"rb");

		if(file)
		{
			fseek(file,0,SEEK_END);
			i=ftell(file);
			fclose(file);

			if(Application->MessageBox((size==i?"Overwrite?":"Previous file has different size! Overwrite?"),"Confirm",MB_YESNO)!=IDYES) return;
		}

		file=fopen(SaveDialogChr->FileName.c_str(),"wb");

		if(file)
		{
			if(size==4096||size==8192)
			{
				if(size==4096)
				{
					memcpy(buf,chr+bankActive,4096);
				}
				else
				{
					memcpy(buf,chr,8192);
				}
			}
			else
			{
				pp=tileActive*16+bankActive;

				for(i=0;i<size;i++)
				{
					buf[i]=chr[pp++];

					if(pp>=4096) pp=0;
				}
			}

			fwrite(buf,size,1,file);
			fclose(file);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	if(Shift.Contains(ssCtrl))
	{
		if(Key=='Z') Undo();

		if(nameSelection.left<0&&nameSelection.top<0)
		{
			if(Key=='X') CopyCHR(true,true);
			if(Key=='C') CopyCHR(true,false);
			if(Key=='V') PasteCHR();
		}
		else
		{
			if(Key=='X') CopyMap(true);
			if(Key=='C') CopyMap(false);
			if(Key=='V') PasteMap();
			if(Key=='F') FillMap();
		}
	}
	else
	{
		if(nameSelection.left<0&&nameSelection.top<0)
		{
			if(Key==VK_DELETE) CopyCHR(false,true);

			if(Key=='H') FormCHREditor->MirrorHorizontal();
			if(Key=='V') FormCHREditor->MirrorVertical();
			if(Key=='R') FormCHREditor->Flip90(false);
			if(Key=='L') FormCHREditor->Flip90(true);
		}
	}

	if(SpeedButtonTypeIn->Down&&PageControlEditor->ActivePage==TabSheetName)
	{
		if(nameSelection.left>=0)
		{
			switch(Key)
			{
			case VK_BACK:
			case VK_LEFT:  if(nameSelection.left>0 ) --nameSelection.left; break;
			case VK_RIGHT: if(nameSelection.left<31) ++nameSelection.left; break;
			case VK_UP:    if(nameSelection.top >0 ) --nameSelection.top;  break;
			case VK_RETURN:
			case VK_DOWN:  if(nameSelection.top <31) ++nameSelection.top;  break;
			}

			nameSelection.right =nameSelection.left+1;
			nameSelection.bottom=nameSelection.top +1;

			UpdateNameTable(-1,-1,false);
		}

		return;
	}

	if(Shift.Contains(ssShift))
	{
		if(Key=='C')
		{
			CopyMap(false);
			CopyMapCodeC();
			return;
		}

		if(Key=='M')
		{
			CopyMetaSpriteCodeC(false);
			return;
		}

		if(Key=='N')
		{
			CopyMetaSpriteCodeC(true);
			return;
		}
	}

	if(PageControlEditor->ActivePage==TabSheetName)
	{
		if(Key==VK_LEFT)  NameTableScrollLeft (Shift.Contains(ssCtrl));
		if(Key==VK_RIGHT) NameTableScrollRight(Shift.Contains(ssCtrl));
		if(Key==VK_UP)    NameTableScrollUp   (Shift.Contains(ssCtrl));
		if(Key==VK_DOWN)  NameTableScrollDown (Shift.Contains(ssCtrl));
	}
	else
	{
		if(Key==VK_OEM_4) SpeedButtonPrevMetaSpriteClick(Sender);// [
		if(Key==VK_OEM_6) SpeedButtonNextMetaSpriteClick(Sender);// ]

		if(Key==VK_LEFT)  MoveSprite(-1, 0);
		if(Key==VK_RIGHT) MoveSprite( 1, 0);
		if(Key==VK_UP)    MoveSprite( 0,-1);
		if(Key==VK_DOWN)  MoveSprite( 0, 1);
	}

	if(Key=='A')
	{
		SpeedButtonChecker->Down^=true;
		SpeedButtonCheckerClick(Sender);
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::PutDataToClipboardASMClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	strcpy(str,".db ");

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			sprintf(buf,"$%2.2x%c",bgPal[i][j],i*4+j<15?',':'\n');
			strcat(str,buf);
		}
	}

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesDblClick(TObject *Sender)
{
	FormCHREditor->Show();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormDestroy(TObject *Sender)
{
	char path[MAX_PATH];
	int len;

	GetCurrentDirectory(len,path);
	reg_save_str(regWorkingDirectory,path);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageNameMouseLeave(TObject *Sender)
{
	nameXC=-1;
	nameYC=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesMouseLeave(TObject *Sender)
{
	tileXC=-1;
	tileYC=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageTilesMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int i,j,tx,ty;
	int xs,ys,wdt,hgt;

	if(SpeedButtonTypeIn->Down) return;

	if(X>=0&&X<256&&Y>=0&&Y<256)
	{
		tx=X/16;
		ty=Y/16;
		tileXC=tx;
		tileYC=ty;

		if(Shift.Contains(ssShift)&&!Shift.Contains(ssCtrl))
		{
			if(Shift.Contains(ssLeft))
			{
				chrSelection.right =tx+(tx>=chrSelection.left?1:0);
				chrSelection.bottom=ty+(ty>=chrSelection.top ?1:0);

				if(abs(chrSelection.left-chrSelection.right)>1||abs(chrSelection.top-chrSelection.bottom)>1)
				{
					nameSelection.left=-1;
					nameSelection.right=-1;

					UpdateNameTable(-1,-1,true);
				}

				for(i=0;i<256;i++) chrSelected[i]=false;

				xs=chrSelection.left<chrSelection.right ?chrSelection.left:chrSelection.right;
				ys=chrSelection.top <chrSelection.bottom?chrSelection.top :chrSelection.bottom;

				wdt=abs(chrSelection.right -chrSelection.left);
				hgt=abs(chrSelection.bottom-chrSelection.top);

				for(i=0;i<hgt;i++)
				{
					for(j=0;j<wdt;j++)
					{
						chrSelected[(i+ys)*16+j+xs]=true;
					}
				}

				chrSelectRect=true;
			}
		}

		if(Shift.Contains(ssCtrl)&&(Shift.Contains(ssLeft)||Shift.Contains(ssRight)))
		{
			chrSelected[ty*16+tx]=Shift.Contains(ssLeft);
			chrSelectRect=false;
		}
	}
	else
	{
		tileXC=-1;
		tileYC=-1;
	}

	UpdateTiles(false);
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectedBlockToClipboardASMClick(TObject *Sender)
{
	CopyMapCodeASM();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRClearClick(TObject *Sender)
{
	int i;

	if(Application->MessageBox("Do you really want to clear 8K CHR?","Confirm",MB_YESNO)==ID_YES)
	{
		SetUndo();
		for(i=0;i<8192;i++) chr[i]=0;
		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonChrBank1Click(TObject *Sender)
{
	bankActive=SpeedButtonChrBank1->Down?0:4096;
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRRemoveDoublesClick(TObject *Sender)
{
	RemoveDoublesUnused(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int i,x,y,col;

	y=0;
	x=16;

	palHover=-1;
	palColHover=-1;
	colHover=-1;

	for(i=0;i<4;i++)
	{
		if(Y>=y&&Y<y+20)
		{
			if(X>=x   &&X<x+20)
			{
				palHover=i;
				palColHover=0;
				colHover=-1;
			}
			if(X>=x+20&&X<x+40)
			{
				palHover=i;
				palColHover=1;
				colHover=-1;
			}
			if(X>=x+40&&X<x+60)
			{
				palHover=i;
				palColHover=2;
				colHover=-1;
			}
			if(X>=x+60&&X<x+80)
			{
				palHover=i;
				palColHover=3;
				colHover=-1;
			}
		}
		x+=128-16;
		if(x>128)
		{
			x=16;
			y=32;
		}
	}

	if(X>=0&&X<14*16&&Y>=64&&Y<64+4*16)
	{
		colHover=X/16+(Y-64)/16*16;
		palHover=-1;
		palColHover=-1;
	}

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxPalMouseLeave(TObject *Sender)
{
	palHover=-1;
	palColHover=-1;
	colHover=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSaveSelClick(TObject *Sender)
{
	FILE *file;
	int i,size;

	if(SaveDialogChrSelection->Execute())
	{
		size=0;

		for(i=0;i<256;i++) if(chrSelected[i]) size+=16;

		file=fopen(SaveDialogChrSelection->FileName.c_str(),"rb");

		if(file)
		{
			fseek(file,0,SEEK_END);
			i=ftell(file);
			fclose(file);

			if(Application->MessageBox((size==i?"Overwrite?":"Previous file has different size! Overwrite?"),"Confirm",MB_YESNO)!=IDYES) return;
		}

		file=fopen(SaveDialogChrSelection->FileName.c_str(),"wb");

		if(file)
		{
			for(i=0;i<256;i++)
			{
				if(chrSelected[i]) fwrite(chr+i*16+bankActive,16,1,file);
			}

			fclose(file);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRInterleaveClick(TObject *Sender)
{
	InterleaveCHR(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRDeinterleaveClick(TObject *Sender)
{
	InterleaveCHR(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSwapColorsClick(TObject *Sender)
{
	bool swap[256*2];
	int i,j,k,pp,col,bit;
	unsigned char paltemp[4];

	FormSwapColors->ShowModal();

	if(FormSwapColors->Swap)
	{
		SetUndo();

		for(i=0;i<256*2;i++) swap[i]=false;

		if(FormSwapColors->Selection)
		{
			for(i=0;i<256;i++) swap[(bankActive/4096*256)+i]=chrSelected[i];
		}
		else
		{
			if(FormSwapColors->WholeCHR)
			{
				for(i=0;i<256*2;i++) swap[i]=true;
			}
			else
			{
				for(i=0;i<256;i++) swap[(bankActive/4096*256)+i]=true;
			}
		}

		for(i=0;i<256*2;i++)
		{
			if(swap[i])
			{
				pp=i*16;

				for(j=0;j<8;j++)
				{
					for(k=0;k<8;k++)
					{
						bit=1<<k;
						col=((chr[pp]&bit)?1:0)|((chr[pp+8]&bit)?2:0);
						col=FormSwapColors->Map[col];
						chr[pp]=(chr[pp]&~bit)|((col&1)<<k);
						chr[pp+8]=(chr[pp+8]&~bit)|((col>>1)<<k);
					}
					pp++;
				}
			}
		}

		if(FormSwapColors->RemapPalette)
		{
			for(i=0;i<4;i++)
			{
				for(j=0;j<4;j++) paltemp[FormSwapColors->Map[j]]=bgPal[i][j];
				for(j=0;j<4;j++) bgPal[i][j]=paltemp[j];
			}
			bgPal[1][0]=bgPal[0][0];
			bgPal[2][0]=bgPal[0][0];
			bgPal[3][0]=bgPal[0][0];
		}

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		DrawPalettes();
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MAddOffsetClick(TObject *Sender)
{
	int i;

	FormNameOffset->ShowModal();

	if(FormNameOffset->MakeOffset)
	{
		SetUndo();

		for(i=0;i<960;i++)
		{
			if(nameTable[i]>=FormNameOffset->From&&nameTable[i]<=FormNameOffset->To) nameTable[i]+=FormNameOffset->Offset;
		}

		UpdateNameTable(-1,-1,true);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskBClick(TObject *Sender)
{
	ppuMask^=0x80;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskGClick(TObject *Sender)
{
	ppuMask^=0x40;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskRClick(TObject *Sender)
{
	ppuMask^=0x20;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMaskMClick(TObject *Sender)
{
	ppuMask^=0x01;
	UpdateRGBM();
	palette_calc();
	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	DrawPalettes();
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportNametableBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i,j,x,y;

	SaveDialogImage->FileName=RemoveExt(SaveDialogName->FileName)+".bmp";

	if(SaveDialogImage->Execute())
	{
		picture=new TPicture();
		bmp=new Graphics::TBitmap();
		bmp->SetSize(256,240);
		bmp->PixelFormat=pf4bit;
		SetBMPPalette(bmp);
		picture->Bitmap=bmp;

		y=0;

		for(i=0;i<30;i++)
		{
			x=0;

			for(j=0;j<32;j++)
			{
				DrawSmallTile16(picture,x,y,nameTable[i*32+j],attr_get(j,i),j,i);
				x+=8;
			}

			y+=8;
		}

		picture->SaveToFile(SaveDialogImage->FileName);

		delete bmp;
		delete picture;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportTilesetBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i,x,y;

	SaveDialogImage->FileName=RemoveExt(SaveDialogChr->FileName)+".bmp";

	if(SaveDialogImage->Execute())
	{
		picture=new TPicture();
		bmp=new Graphics::TBitmap();
		bmp->SetSize(128,128);
		bmp->PixelFormat=pf4bit;
		SetBMPPalette(bmp);
		picture->Bitmap=bmp;

		x=0;
		y=0;

		for(i=0;i<256;i++)
		{
			DrawSmallTile16(picture,x,y,i,palActive,-1,-1);
			x+=8;
			if(x>=128)
			{
				x=0;
				y+=8;
			}
		}

		picture->SaveToFile(SaveDialogImage->FileName);

		delete bmp;
		delete picture;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PutDataToClipboardCClick(TObject *Sender)
{
	char str[1024],buf[1024];
	int i,j;

	strcpy(str,"const unsigned char palette[16]={ ");

	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			sprintf(buf,"0x%2.2x%c",bgPal[i][j],i*4+j<15?',':' ');
			strcat(str,buf);
		}
	}

	strcat(str,"};\n\n");

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutSelectedBlockToClipboardCClick(TObject *Sender)
{
	CopyMapCodeC();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonCheckerClick(TObject *Sender)
{
	UpdateNameTable(-1,-1,true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutMetaSpriteToClipboardCClick(TObject *Sender)
{
	CopyMetaSpriteCodeC(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MPutMetaSpriteToClipboardHFlipCClick(TObject *Sender)
{
	CopyMetaSpriteCodeC(true);
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::MSaveAttributesClick(TObject *Sender)
{
	AnsiString name;

	SaveDialogAttr->FileName=RemoveExt(SaveDialogAttr->FileName);
	SaveDialogAttr->Title="Save attributes only";

	if(SaveDialogAttr->Execute())
	{
		name=RemoveExt(SaveDialogAttr->FileName);

		switch(SaveDialogAttr->FilterIndex)
		{
		case 2: name+=".bin"; break;
		case 3: name+=".rle"; break;
		case 4: name+=".h"; break;
		default: name+=".atr";
		}

		if(FileExists(name))
		{
			if(Application->MessageBox(("File "+name+" is already exist, overwrite?").c_str(),"Confirm",MB_YESNO)!=IDYES) return;
		}

		save_data(name.c_str(),attrTable,64,SaveDialogAttr->FilterIndex);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFillNumbersClick(TObject *Sender)
{
	int i,j,k,pp;

	if(Application->MessageBox("Do you really want to fill 8K CHR with numbers?","Confirm",MB_YESNO)==ID_YES)
	{
		SetUndo();

		pp=0;

		for(i=0;i<16;i++)
		{
			for(j=0;j<16;j++)
			{
				for(k=0;k<16;k++)
				{
					chr[pp++]=(smallnums_data[(i<<4)+k]&0xf0)|(smallnums_data[(j<<4)+k]&0x0f);
				}
			}
		}

		memcpy(&chr[4096],chr,4096);

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFindDoublesClick(TObject *Sender)
{
	FindDoublesUnused(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportBMPNametableClick(TObject *Sender)
{
	FILE *file;
	unsigned char *data;
	int i,j,k,l,x,y,size,wdt,hgt,pp,ps,off,ptr,cnt,bpp,fi,min,max,id,rx,gx,bx,pwdt;
	unsigned char tile[16];
	unsigned char chrt[4096];
	unsigned char namet[960];
	unsigned char bmp[240][256];
	int attr[4];
	bool add;

	OpenDialogImport->Title="Import BMP file as nametable";
	OpenDialogImport->Filter="Windows bitmap files (*.bmp)|*.bmp|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="bmp";

	if(OpenDialogImport->Execute())
	{
		file=fopen(OpenDialogImport->FileName.c_str(),"rb");

		if(file)
		{
			size=get_file_size(file);
			data=(unsigned char*)malloc(size);
			fread(data,size,1,file);
			fclose(file);

			bpp=data[28];

			if(data[0]!='B'||data[1]!='M'||(bpp!=4&&bpp!=8)||data[29]!=0||read_dword(&data[30]))
			{
				Application->MessageBox("Wrong BMP format, only 4bpp and 8bpp files without compression are supported","Error",MB_OK);
				free(data);
				return;
			}

			off=read_dword(&data[10]);
			wdt=read_dword(&data[18]);
			hgt=read_dword(&data[22]);

			if(wdt>256||(hgt>240||hgt<-240))
			{
				Application->MessageBox("BMP should not be larger than 256x240 pixels","Error",MB_OK);
				free(data);
				return;
			}

			memset(chrt ,0,sizeof(chrt));
			memset(namet,0,sizeof(namet));
			ptr=0;
			cnt=0;

			//find similar colors in NES palette

			for(i=0;i<16;i++)
			{
				min=0x01000000;
				id=0;

				for(j=0;j<64;j++)
				{
					rx=((outPalette[j]>>16)&255)-data[i*4+54];
					gx=((outPalette[j]>>8 )&255)-data[i*4+55];
					bx=((outPalette[j]    )&255)-data[i*4+56];

					fi=30*rx*rx+59*gx*gx+11*bx*bx;

					if(fi<min)
					{
						min=fi;
						id=j;
					}
				}

				if(i!=4&&i!=8&&i!=12) bgPal[i>>2][i&3]=id;
			}

			bgPal[1][0]=bgPal[0][0];
			bgPal[2][0]=bgPal[0][0];
			bgPal[3][0]=bgPal[0][0];

			pal_validate();

			//put bitmap into an array to make it easier to work with

			pwdt=wdt;

			if(pwdt&3) pwdt=(pwdt&~3)+4;//pad rows to four bytes

			for(i=0;i<240;++i)
			{
				for(j=0;j<256;++j) bmp[i][j]=0;
			}

			switch(bpp)
			{
			case 4:
				for(i=0;i<hgt;++i)
				{
					ps=off+(hgt-1-i)*(pwdt/2);

					for(j=0;j<wdt;j+=2)
					{
						bmp[i][j+0]=data[ps]>>4;

						if(j+1<wdt) bmp[i][j+1]=data[ps]&15;

						++ps;
					}
				}
				break;

			case 8:
				for(i=0;i<hgt;++i)
				{
					ps=off+(hgt-1-i)*pwdt;

					for(j=0;j<wdt;++j) bmp[i][j]=data[ps++];
				}
				break;
			}

			free(data);

			//convert graphics into tiles

			if(wdt&7) wdt=(wdt&~7)+8;
			if(hgt&7) hgt=(hgt&~7)+8;

			for(i=0;i<hgt/8;++i)
			{
				for(j=0;j<wdt/8;++j)
				{
					memset(tile,0,sizeof(tile));

					pp=0;

					y=i<<3;

					for(k=0;k<8;++k)
					{
						x=j<<3;

						for(l=0;l<8;++l)
						{
							tile[pp+0]|=bmp[y][x]&1?1<<(7-l):0;
							tile[pp+8]|=bmp[y][x]&2?1<<(7-l):0;
							x++;
						}

						++pp;
						++y;
					}

					add=true;

					for(k=0;k<(cnt<256?cnt:256);++k)
					{
						if(memcmp(tile,&chrt[k*16],16)==0)
						{
							namet[i*32+j]=k;
							add=false;
							break;
						}
					}

					if(add)
					{
						if(cnt<256)
						{
							memcpy(&chrt[ptr],tile,16);
							ptr+=16;
							namet[i*32+j]=cnt;
						}

						cnt++;
					}
				}
			}

			if(cnt>=256) Application->MessageBox(("Too many unique characters ("+IntToStr(cnt)+"), "+IntToStr(cnt-256)+" skipped").c_str(),"Warning",MB_OK);

			memcpy(chr+bankActive,chrt,4096);
			memcpy(nameTable,namet,960);

			//try to set proper attributes

			for(i=0;i<hgt;i+=16)
			{
				for(j=0;j<wdt;j+=16)
				{
					for(k=0;k<4;++k) attr[k]=0;

					for(k=i;k<i+16;++k)
					{
						for(l=j;l<j+16;++l)
						{
							if(bmp[k][l]&3) attr[bmp[k][l]>>2]++;
						}
					}

					id=0;
					max=0;

					for(k=0;k<4;++k)
					{
						if(attr[k]>max)
						{
							max=attr[k];
							id=k;
						}
					}

					attr_set(j>>3,i>>3,id);
				}
			}
		}

		DrawPalettes();
		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportNESClick(TObject *Sender)
{
	FILE *file;
	unsigned char *data;

	OpenDialogImport->Title="Import NES file";
	OpenDialogImport->Filter="NES cartridge dump|*.nes|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="nes";

	if(OpenDialogImport->Execute())
	{
		FormBank->Caption="Import 8K CHR bank from NES file";
		FormBank->FileName=OpenDialogImport->FileName;
		FormBank->ShowModal();

		if(FormBank->OK)
		{
			file=fopen(OpenDialogImport->FileName.c_str(),"rb");

			if(file)
			{
				fseek(file,0,SEEK_END);
				fseek(file,16+FormBank->PRG*16384+FormBank->Bank*8192,SEEK_SET);
				fread(chr,8192,1,file);
				fclose(file);
			}

			UpdateTiles(true);
			UpdateNameTable(-1,-1,true);
			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportNESClick(TObject *Sender)
{
	FILE *file;
	unsigned char *nes;
	int size;

	if(SaveDialogExportNES->Execute())
	{
		FormBank->Caption="Export 8K CHR bank to NES file";
		FormBank->FileName=SaveDialogExportNES->FileName;
		FormBank->ShowModal();

		if(FormBank->OK)
		{
			file=fopen(SaveDialogExportNES->FileName.c_str(),"rb");

			if(!file) return;

			fseek(file,0,SEEK_END);
			size=ftell(file);
			fseek(file,0,SEEK_SET);

			nes=(unsigned char*)malloc(size);
			fread(nes,size,1,file);
			fclose(file);

			file=fopen((SaveDialogExportNES->FileName+".bak").c_str(),"wb");

			if(!file)
			{
				free(nes);
				return;
			}

			fwrite(nes,size,1,file);
			fclose(file);

			memcpy(nes+16+FormBank->PRG*16384+FormBank->Bank*8192,chr,8192);

			file=fopen(SaveDialogExportNES->FileName.c_str(),"wb");

			if(!file)
			{
				free(nes);
				return;
			}

			fwrite(nes,size,1,file);
			fclose(file);
			free(nes);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportBMPTilesetClick(TObject *Sender)
{
	FILE *file;
	unsigned char *data;
	int i,j,k,l,size,wdt,hgt,pp,ps,off,ptr,bpp;
	unsigned char tile[16];

	OpenDialogImport->Title="Import BMP file as tileset";
	OpenDialogImport->Filter="Windows bitmap files (*.bmp)|*.bmp|All files (*.*)|*.*";
	OpenDialogImport->DefaultExt="bmp";

	if(OpenDialogImport->Execute())
	{
		file=fopen(OpenDialogImport->FileName.c_str(),"rb");

		if(file)
		{
			size=get_file_size(file);
			data=(unsigned char*)malloc(size);
			fread(data,size,1,file);
			fclose(file);

			bpp=data[28];

			if(data[0]!='B'||data[1]!='M'||(bpp!=4&&bpp!=8)||data[29]!=0||read_dword(&data[30]))
			{
				Application->MessageBox("Wrong BMP format, only 4bpp and 8bpp files without compression are supported","Error",MB_OK);
				free(data);
				return;
			}

			off=read_dword(&data[10]);
			wdt=read_dword(&data[18]);
			hgt=read_dword(&data[22]);

			if(wdt!=128||(hgt!=128&&hgt!=-128))
			{
				Application->MessageBox("BMP should be 128x128 pixels","Error",MB_OK);
				free(data);
				return;
			}

			ptr=bankActive;

			for(i=0;i<16;i++)
			{
				for(j=0;j<16;j++)
				{
					memset(tile,0,sizeof(tile));
					pp=0;

					if(bpp==4)
					{
						for(k=0;k<8;k++)
						{
							/*if(hgt>0) ps=off+(i*8+k)*128+j*4; else */ps=off+(127-i*8-k)*64+j*4;
							for(l=0;l<8;l++)
							{
								tile[pp]|=(data[ps]&(l&1?0x01:0x10))?1<<(7-l):0;
								tile[pp+8]|=(data[ps]&(l&1?0x02:0x20))?1<<(7-l):0;
								if(l&1) ps++;
							}
							pp++;
						}
					}
					else
					{
						for(k=0;k<8;k++)
						{
							/*if(hgt>0) ps=off+(i*8+k)*128+j*4; else */ps=off+(127-i*8-k)*128+j*8;
							for(l=0;l<8;l++)
							{
								tile[pp]|=(data[ps]&1)?1<<(7-l):0;
								tile[pp+8]|=(data[ps]&2)?1<<(7-l):0;
								ps++;
							}
							pp++;
						}
					}

					memcpy(&chr[ptr],tile,16);
					ptr+=16;
				}
			}

			free(data);
		}

		UpdateTiles(true);
		UpdateNameTable(-1,-1,true);
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRSwapBanksClick(TObject *Sender)
{
	unsigned char temp[4096];

	memcpy(temp,chr,4096);
	memcpy(chr,chr+4096,4096);
	memcpy(chr+4096,temp,4096);

	UpdateTiles(true);
	UpdateNameTable(-1,-1,true);
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenClick(TObject *Sender)
{
	if(OpenDialogAll->Execute()) OpenAll(OpenDialogAll->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveClick(TObject *Sender)
{
	MCHRSaveClick(this);
	if(SaveDialogName->FileName=="") SaveDialogName->FileName=SaveDialogChr->FileName;
	MSaveNameTableAttributesClick(this);
	if(SaveDialogPal->FileName=="") SaveDialogPal->FileName=SaveDialogName->FileName;
	MSavePalettesClick(this);
	MSaveMetaSpriteBankClick(this);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MLoadSessionClick(TObject *Sender)
{
	if(OpenDialogSession->Execute()) LoadSession(OpenDialogSession->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveSessionClick(TObject *Sender)
{
	if(SaveDialogSession->Execute()) SaveSession(SaveDialogSession->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportPaletteBMPClick(TObject *Sender)
{
	TPicture *picture;
	Graphics::TBitmap *bmp;
	int i;
	unsigned char* dst;

	SaveDialogImage->FileName=RemoveExt(SaveDialogPal->FileName)+".bmp";

	if(SaveDialogImage->Execute())
	{
		picture=new TPicture();
		bmp=new Graphics::TBitmap();
		bmp->SetSize(16,1);
		bmp->PixelFormat=pf4bit;
		SetBMPPalette(bmp);
		picture->Bitmap=bmp;

		dst=(unsigned char*)picture->Bitmap->ScanLine[0];

		for(i=0;i<16;i+=2) *dst++=((i+1)|(i<<4));

		picture->SaveToFile(SaveDialogImage->FileName);

		delete bmp;
		delete picture;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonTypeInClick(TObject *Sender)
{
	if(SpeedButtonTypeIn->Down)
	{
		if(nameSelection.left>=0)
		{
			nameSelection.right =nameSelection.left+1;
			nameSelection.bottom=nameSelection.top +1;

			UpdateNameTable(-1,-1,true);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyPress(TObject *Sender, char &Key)
{
	if(SpeedButtonTypeIn->Down)
	{
		if(Key>=32)
		{
			NameTableTypeIn(Key-32);
		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::PageControlEditorChange(TObject *Sender)
{
	UpdateMetaSprite();
	ActiveControl=NULL;//to prevent ListBoxSpriteList grab focus while Nametable tab is active
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteDragOver(TObject *Sender,
TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	if(Source->InheritsFrom(__classid(TImage))) Accept=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteDragDrop(TObject *Sender,
TObject *Source, int X, int Y)
{
	int i,pp;

	pp=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[pp]==255)
		{
			metaSprites[pp+0]=Y/2;
			metaSprites[pp+1]=tileActive;
			metaSprites[pp+2]=palActive;
			metaSprites[pp+3]=X/2;

			if(SpeedButtonSpriteSnap->Down) sprite_snap(i);

			ListBoxSpriteList->ItemIndex=i;
			SelectSprite(i);

			break;
		}

		pp+=4;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteEndDrag(TObject *Sender,
TObject *Target, int X, int Y)
{
	if(Target) UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonPrevMetaSpriteClick(TObject *Sender)
{
	if(metaSpriteActive)
	{
		--metaSpriteActive;
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonNextMetaSpriteClick(TObject *Sender)
{
	if(metaSpriteActive<255)
	{
		++metaSpriteActive;
		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonClearMetaSpriteClick(TObject *Sender)
{
	int i,off;

	if(Application->MessageBox("Metasprite will be cleared out!","Are you sure?",MB_YESNO)==IDYES)
	{
		off=metaSpriteActive*64*4;

		for(i=0;i<64*4;i+=4) metaSprites[off+i]=255;

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteDelClick(TObject *Sender)
{
	int i,off;

	if(Application->MessageBox("Sprite will be deleted!","Are you sure?",MB_YESNO)==IDYES)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		for(i=0;i<4;++i) metaSprites[off+i]=255;

		squeeze_sprites();

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSpriteListClick(TObject *Sender)
{
	SelectSprite(ListBoxSpriteList->ItemIndex);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonFrameSelectedClick(TObject *Sender)
{
	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteUpClick(TObject *Sender)
{
	unsigned char temp[4];
	int off;

	off=metaSpriteActive*64*4+spriteActive*4;

	if(spriteActive>0)
	{
		if(metaSprites[off]<255)
		{
			memcpy( temp              ,&metaSprites[off-4],4);
			memcpy(&metaSprites[off-4],&metaSprites[off  ],4);
			memcpy(&metaSprites[off  ], temp              ,4);

			--spriteActive;

			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteDownClick(TObject *Sender)
{
	unsigned char temp[4];
	int off;

	off=metaSpriteActive*64*4+spriteActive*4;

	if(spriteActive<63)
	{
		if(metaSprites[off+4]<255)
		{
			memcpy( temp              ,&metaSprites[off  ],4);
			memcpy(&metaSprites[off  ],&metaSprites[off+4],4);
			memcpy(&metaSprites[off+4], temp              ,4);

			++spriteActive;

			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSpriteListKeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	FormKeyDown(Sender,Key,Shift);
	Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int i,off;

	if(Shift.Contains(ssLeft)&&!Shift.Contains(ssCtrl))
	{
		i=get_sprite_id(X,Y);

		if(i>=0)
		{
			SelectSprite(i);
		}
	}

	if(Shift.Contains(ssRight))
	{
		i=get_sprite_id(X,Y);

		if(i>=0)
		{
			SelectSprite(i);
		}

		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			spriteDragX=metaSprites[off+3]-X/2;
			spriteDragY=metaSprites[off+0]-Y/2;
		}

		spriteDrag=i;
	}

	ImageMetaSpriteMouseMove(Sender,Shift,X,Y);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseLeave(TObject *Sender)
{
	spriteHover=-1;
	spriteDrag=-1;
	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int x,y,off;

	if(Shift.Contains(ssRight)&&spriteDrag>=0)
	{
		off=metaSpriteActive*64*4+spriteDrag*4;

		if(metaSprites[off]<255)
		{
			x=X/2+spriteDragX;
			y=Y/2+spriteDragY;

			if(x<0)   x=0;
			if(x>120) x=120;
			if(y<0)   y=0;
			if(y>120) y=120;

			metaSprites[off+3]=x;
			metaSprites[off+0]=y;

			UpdateMetaSprite();
		}
	}

	if(Shift.Contains(ssLeft)&&Shift.Contains(ssCtrl))
	{
		spriteGridX=(X/2)&~7;
		spriteGridY=(Y/2)&~7;

		if(spriteGridX<8) spriteGridX=8;
		if(spriteGridY<8) spriteGridY=8;

		UpdateMetaSprite();
	}

	spriteHover=get_sprite_id(X,Y);

	UpdateStats();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ImageMetaSpriteMouseUp(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(SpeedButtonSpriteSnap->Down)
	{
		sprite_snap(spriteActive);

		UpdateMetaSprite();
	}

	spriteDrag=-1;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenMetaSpriteBankClick(TObject *Sender)
{
	if(OpenDialogMetaSpriteBank->Execute())
	{
		if(OpenMetaSprites(OpenDialogMetaSpriteBank->FileName)) SaveDialogMetaSpriteBank->FileName=OpenDialogMetaSpriteBank->FileName;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveMetaSpriteBankClick(TObject *Sender)
{
	FILE *file;
	unsigned char data[2];

	if(SaveDialogMetaSpriteBank->Execute())
	{
		file=fopen(SaveDialogMetaSpriteBank->FileName.c_str(),"wb");

		if(file)
		{
			data[0]=spriteGridX;
			data[1]=spriteGridY;

			fwrite(data,2,1,file);
			fwrite(metaSprites,256*64*4,1,file);
			fclose(file);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutBankCToClipboardClick(TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,pp,off,size;
	bool first;

	sprintf(str,"const unsigned char* const metaspritelist[]={ //256 pointers to metasprite data\n");

	pp=0;

	for(i=0;i<256;++i)
	{
		off=i*64*4;

		if(metaSprites[off]<255)
		{
			if(pp) strcat(str,",\n");

			sprintf(buf,"metaspritedata+%i",pp);
			strcat(str,buf);

			size=1;

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) break;

				off+=4;
				size+=4;
			}

			pp+=size;
		}
		else
		{
			if(pp) strcat(str,",\n");

			sprintf(buf,"metaspritedata+0");
			strcat(str,buf);
		}
	}

	strcat(str,"\n};\n\n//metasprite data array\n\n");
	strcat(str,"const unsigned char metaspritedata[]={\n");

	first=true;

	for(i=0;i<256;++i)
	{

		off=i*64*4;

		if(metaSprites[off]<255)
		{
			if(!first) strcat(str,",\n");

			first=false;

			sprintf(buf,"//metasprite %i\n",i);
			strcat(str,buf);

			for(j=0;j<64;++j)
			{
				if(metaSprites[off]==255) continue;

				if(j) strcat(str,",\n");

				sprintf(buf,"%i,%i,0x%2.2x,%i",metaSprites[off+3]-spriteGridX,metaSprites[off+0]-spriteGridY,metaSprites[off+1],metaSprites[off+2]&3);

				strcat(str,buf);

				if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
				if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

				off+=4;
			}

			strcat(str,",\n128");
		}
	}

	strcat(str,"\n};\n\n");

	Clipboard()->SetTextBuf(str);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MMetaSpritePutCToClipboardClick(TObject *Sender)
{
	char str[65536],buf[1024];
	int i,j,x,y,off;

	off=metaSpriteActive*64*4;

	if(metaSprites[off]<255)
	{
		sprintf(str,"const unsigned char metasprite[]={\n");

		for(i=0;i<64;++i)
		{
			if(i) strcat(str,",\n");

			if(metaSprites[off]==255) break;

			sprintf(buf,"%i,%i,0x%2.2x,%i",metaSprites[off+3]-spriteGridX,metaSprites[off]-spriteGridY,metaSprites[off+1],metaSprites[off+2]&3);

			strcat(str,buf);

			if(metaSprites[off+2]&OAM_FLIP_H) strcat(str,"|OAM_FLIP_H");
			if(metaSprites[off+2]&OAM_FLIP_V) strcat(str,"|OAM_FLIP_V");

			off+=4;
		}

		strcat(str,"128\n};\n\n");

		Clipboard()->SetTextBuf(str);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteHFlipClick(TObject *Sender)
{
	int off;

	if(spriteActive>=0)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			metaSprites[off+2]^=OAM_FLIP_H;

			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSpriteVFlipClick(TObject *Sender)
{
	int off;

	if(spriteActive>=0)
	{
		off=metaSpriteActive*64*4+spriteActive*4;

		if(metaSprites[off]<255)
		{
			metaSprites[off+2]^=OAM_FLIP_V;

			UpdateMetaSprite();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteCopyClick(TObject *Sender)
{
	memcpy(metaSpriteCopy,&metaSprites[metaSpriteActive*64*4],sizeof(metaSpriteCopy));
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpritePasteClick(TObject *Sender)
{
	if(metaSpriteCopy[0]<255)
	{
		memcpy(&metaSprites[metaSpriteActive*64*4],metaSpriteCopy,sizeof(metaSpriteCopy));

		UpdateMetaSprite();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteHFlipClick(TObject *Sender)
{
	int i,x,off;

	off=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[off]==255) break;

		x=-8-(metaSprites[off+3]-spriteGridX);

		metaSprites[off+3]=spriteGridX+x;
		metaSprites[off+2]^=OAM_FLIP_H;

		off+=4;
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonMetaSpriteVFlipClick(TObject *Sender)
{
	int i,y,off;

	off=metaSpriteActive*64*4;

	for(i=0;i<64;++i)
	{
		if(metaSprites[off]==255) break;

		y=-8-(metaSprites[off+0]-spriteGridY);

		metaSprites[off+0]=spriteGridY+y;
		metaSprites[off+2]^=OAM_FLIP_V;

		off+=4;
	}

	UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRFindUnusedClick(TObject *Sender)
{
	FindDoublesUnused(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MCHRRemoveUnusedClick(TObject *Sender)
{
	RemoveDoublesUnused(true);
}
//---------------------------------------------------------------------------

