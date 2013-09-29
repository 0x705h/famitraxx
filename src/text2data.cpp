// ported to gcc by alekhin0w.
// original code by Shiru

#define _CRT_SECURE_NO_WARNINGS	//to allow fopen etc be used in VC2008 Express without warnings

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define OUT_NESASM	0
#define OUT_CA65	1
#define OUT_ASM6	2



char *text;
unsigned char data[65536];
int textSize;
int textPtr;
int dataPtr;

char db[6];
char dw[6];
char local;



static struct {
	int value[256];
	int len;
	int loop;
} envList[128*3];

static struct {
	int number;
	int envVolume;
	int envArpeggio;
	int envPitch;
	int duty;
} insList[64];

static struct {
	unsigned char note;
	unsigned char ins;
	unsigned char fx;
	unsigned char param;
} chnList[5][128*256];

static struct {
	int off;
	int size;
} dpcmList[64];

static struct {
	int off;
	int size;
	int pitch;
	int loop;
} dpcmInsList[12];

int rowList[128*256];



void setptr(int ptr)
{
	textPtr=ptr;
}



void nextline(void)
{
	while(textPtr<textSize)
	{
		if(text[textPtr]=='\n')
		{
			while(text[textPtr]=='\n') textPtr++;
			break;
		}
		textPtr++;
	}
}



int find(const char *str,int size,int local)
{
	while(textPtr<size-(int)strlen(str))
	{
		if(local&&text[textPtr]=='[') return -1;
		if(!strncmp(text+textPtr,str,strlen(str)))
		{
			textPtr+=strlen(str);
			return textPtr;
		}
		textPtr++;
	}

	return -1;
}



int readnum(void)
{
	int num,sign;

	num=0;
	sign=1;

	while(textPtr<textSize)
	{
		if(text[textPtr]!='-'&&!(text[textPtr]>='0'&&text[textPtr]<='9')) break;
		if(text[textPtr]=='-') sign=-1; else num=num*10+text[textPtr]-'0';
		textPtr++;
	}

	return num*sign;
}



int hex(char c)
{
	if(c>='a'&&c<='f') return c-'a'+10;
	if(c>='A'&&c<='F') return c-'A'+10;
	if(c>='0'&&c<='9') return c-'0';
	return 0;
}



int findval(const char *str,int size,int local,bool skipLoop)
{
	int off;

	off=find(str,size,local);

	if(off<0) return 0x00ffffff;

	while(off<textSize)
	{
		if(skipLoop&&text[off]=='|')
		{
			off++;
			continue;
		}

		if(text[off]!=' '&&text[off]!='='&&text[off]!='\n') break;
		off++;
	}

	setptr(off);

	return readnum();
}



void cleardata(void)
{
	dataPtr=0;
}



void putdata(unsigned char n)
{
	data[dataPtr++]=n;
}



void fdumpdata(FILE *file)
{
	int i;

	for(i=0;i<dataPtr;i++)
	{
		if(!(i&15)) fprintf(file,"\t%s ",db);
		fprintf(file,"$%2.2x%c",data[i],((i&15)<15)&&(i<dataPtr-1)?',':'\n');
	}

	cleardata();
}



int main(int argc,char* argv[])
{
	char name[1024],str[256];
	char songname[1024];
	FILE *file;
	int i,j,k,off,val,ins,len,prev,note,fx;
	int speed,frames,ptnlen,rowcnt,loop,ptnrow,ptnmin;
	int ptnbreak,songbreak,ref;
	int outsize,dutyoff,frameoff,outtype,inssize,inscnt;
	int blocksize;//not larger than 64, that's limitation of RLE for empty rows
	unsigned char dpcm[16384];
	int samples,dpcmoff,dpcmsize,sampleoff,size;
	bool outdpcm,outins,outinsonly;

	printf("%s\n",argv[0]);

	if(argc<2)
	{
		printf("text2data converter for FamiTone audio library\n");
		printf("by Shiru (shiru@mail.ru) 07'11\n\n\n");
		printf("Usage: text2data.exe text.txt [-ca65 or -asm6][-nodpcm][-noins][-insonly]\n");
		return 0;
	}

	outtype=OUT_NESASM;
	outdpcm=true;
	outins=true;
	outinsonly=false;

	if(argc>2)
	{
		for(i=2;i<argc;i++)
		{
			if(!strcasecmp(argv[i],"-ca65"))    outtype=OUT_CA65;
			if(!strcasecmp(argv[i],"-asm6"))    outtype=OUT_ASM6;
			if(!strcasecmp(argv[i],"-nodpcm"))  outdpcm=false;
			if(!strcasecmp(argv[i],"-noins"))   outins=false;
			if(!strcasecmp(argv[i],"-insonly")) outinsonly=true;
		}
	}

	switch(outtype)
	{
	case OUT_NESASM:
		printf("Output format: NESASM\n");
		strcpy(db,".db");
		strcpy(dw,".dw");
		local='.';
		break;

	case OUT_CA65:
		printf("Output format: CA65\n");
		strcpy(db,".byte");
		strcpy(dw,".word");
		local='@';
		break;

	case OUT_ASM6:
		printf("Output format: Asm6\n");
		strcpy(db,"db");
		strcpy(dw,"dw");
		local='@';
		break;
	}

	file=fopen(argv[1],"rb");

	if(!file)
	{
		printf("Can't open file\n");
		return 1;
	}

	fseek(file,0,SEEK_END);
	textSize=ftell(file)+1;
	fseek(file,0,SEEK_SET);
	text=(char*)malloc(textSize);
	fread(text,textSize-1,1,file);
	fclose(file);
	text[textSize-1]=0;

	for(i=0;i<textSize;i++) if(text[i]=='\r') text[i]='\n';

	memset(envList,0,sizeof(envList));
	memset(insList,0,sizeof(insList));
	memset(chnList,0,sizeof(chnList));

	for(i=0;i<128*256;i++) rowList[i]=-1;

	memset(dpcm,0,16384);
	dpcmoff=0;
	dpcmsize=0;
	memset(dpcmList,0,sizeof(dpcmList));
	for(i=0;i<64;i++) dpcmList[i].off=-1;

	memset(dpcmInsList,0,sizeof(dpcmInsList));

	//get global parameters

	printf("Parsing...\n");
	printf(" global parameters\n");

	speed=findval("Speed",textSize,0,false);
	frames=findval("FramesCount",textSize,0,false);
	ptnlen=findval("PatternLength",textSize,0,false);
	samples=findval("SamplesCount",textSize,0,false);

	blocksize=ptnlen;
	while(blocksize>64) blocksize/=2;
	//blocksize=32;

	setptr(0);
	frameoff=find("[Frame",textSize,0);

	if(frameoff<0)
	{
		printf("Can't find any frames\n");
		free(text);
		return 1;
	}

	//get envelopes

	printf(" envelopes\n");

	for(i=0;i<128*3;i++)
	{
		envList[i].len=0;
		envList[i].loop=-1;

		switch(i/128)
		{
		case 0: sprintf(str,"SequenceVolume%i="  ,i&127); break;
		case 1: sprintf(str,"SequenceArpeggio%i=",i&127); break;
		case 2: sprintf(str,"SequencePitch%i="   ,i&127); break;
		}

		setptr(0);
		off=find(str,frameoff,0);

		if(off>=0)
		{
			while(textPtr<textSize)
			{
				if(text[textPtr]==',') textPtr++;

				if(text[textPtr]=='|')
				{
					envList[i].loop=envList[i].len;
					textPtr++;
				}

				if(text[textPtr]=='\n') break;

				val=readnum();

				if(val==0x00ffffff) break;

				envList[i].value[envList[i].len]=val;

				if(i>=128*2&&envList[i].len)
				{
					envList[i].value[envList[i].len]+=envList[i].value[envList[i].len-1];//accumulation for pitch envelopes, to simplify player
				}

				envList[i].len++;
			}
		}
	}

	//get instruments

	printf(" instruments\n");

	ins=1;
	setptr(0);
	dutyoff=find("[SequencesDuty]",frameoff,0);

	if(dutyoff<0)
	{
		printf("Can't find SequencesDuty section\n");
		free(text);
		return 1;
	}

	inscnt=0;

	for(i=0;i<64;i++)
	{
		setptr(0);
		sprintf(str,"[Instrument%i]",i);
		off=find(str,frameoff,0);

		if(off>=0)
		{
			insList[i].number=ins++;
			insList[i].envVolume=findval("SequenceVolume",frameoff,1,false);

			if(insList[i].envVolume==0x00ffffff)
			{
				printf("Warning! Instrument %i does not have volume envelope\n",i);
			}

			setptr(off);

			insList[i].envArpeggio=findval("SequenceArpeggio",frameoff,1,false);

			setptr(off);

			insList[i].envPitch   =findval("SequencePitch",frameoff,1,false);

			setptr(off);

			off=findval("SequenceDuty",frameoff,1,false);

			if(off!=0x00ffffff)
			{
				sprintf(str,"SequenceDuty%i",off,1);
				setptr(dutyoff);
				insList[i].duty=((findval(str,frameoff,1,true)&3)<<6)|0x30;
			}
			else
			{
				insList[i].duty=0x30;
			}

			inscnt++;
		}
	}

	if(inscnt>32) printf("Warning! Only 32 instruments will be used\n");

	//get channels

	printf(" channels\n");

	rowcnt=0;
	loop=0;
	songbreak=0;
	ptnmin=256;

	for(i=0;i<frames;i++)
	{
		if(songbreak) break;

		setptr(frameoff-10);
		sprintf(str,"[Frame%i]",i);

		if(find(str,textSize,0)<0)
		{
			printf("Can't find Frame%i section\n",i);
			free(text);
			return 1;
		}

		ptnbreak=0;
		rowList[rowcnt]=i;//mark start of every frame
		ptnrow=ptnlen;

		for(j=0;j<ptnlen;j++)
		{
			if(ptnbreak)
			{
				ptnrow=j;
				break;
			}

			nextline();
			textPtr+=3;

			for(k=0;k<5;k++)
			{
				note=255;
				ins=0;
				fx=0;
				val=0;

				switch(text[textPtr])
				{
				case '-': note=63; break;
				case 'C': note=0; break;
				case 'D': note=2; break;
				case 'E': note=4; break;
				case 'F': note=5; break;
				case 'G': note=7; break;
				case 'A': note=9; break;
				case 'B': note=11; break;
				}

				if(note!=255&&note!=63)
				{
					if(text[textPtr+1]=='#') note++;
					if(text[textPtr+2]>='0'&&text[textPtr+2]<='9') note=note+12*(text[textPtr+2]-'0');
					if(k<3) note-=12;
					if(k==3) note&=0x0f;//no need to have octaves for noise channel
					if(k==4) note%=12;//no need to have octaves for DPCM as well
					if(note>60||note<0) printf("Warning! Note is out of supported range, frame %i row %2.2x channel %i\n",i,j,k);
				}

				if(text[textPtr+4]!='.')
				{
					ins=(hex(text[textPtr+4])<<4)+hex(text[textPtr+5]);
					ins=insList[ins].number;
				}

				if(text[textPtr+9]!='.')
				{
					fx=text[textPtr+9];
					val=(hex(text[textPtr+10])<<4)+hex(text[textPtr+11]);
				}

				if(fx=='D')
				{
					ptnbreak=1;
					fx=0;
					val=0;
				}

				if(fx=='B')
				{
					loop=val;

					if(val>i)
					{
						printf("Error! Forward references in Bxx aren't supported! Frame %i row %2.2x channel %i\n",i,j,k);
						exit(1);
					}

					ptnbreak=1;
					songbreak=1;
					fx=0;
					val=0;
				}

				chnList[k][rowcnt].note=note;
				chnList[k][rowcnt].ins=ins;
				chnList[k][rowcnt].fx=fx;
				chnList[k][rowcnt].param=val;

				textPtr+=13;
			}

			rowcnt++;
		}

		if(ptnrow<ptnmin) ptnmin=ptnrow;
	}

	if(blocksize>ptnmin) blocksize=ptnmin;//hack for shortened patterns, works bad

	//get samples

	printf(" samples\n");

	setptr(frameoff);
	sampleoff=find("[Sample",textSize,0);
	if(sampleoff>=0) sampleoff-=7;

	for(i=0;i<64;i++)
	{
		sprintf(str,"[Sample%i]",i+1);

		setptr(sampleoff);
		off=find(str,textSize,0);

		if(off>=0)
		{
			size=findval("SampleSize=",textSize,1,false);
			off=find("SampleData=$",textSize,1);

			dpcmList[i].off=dpcmoff>>6;
			size=0;
			j=0;

			while(off<textSize)
			{
				if(text[off]=='\n') break;
				if(!j)
				{
					val=hex(text[off])<<4;
					j=1;
				}
				else
				{
					val|=hex(text[off]);
					dpcm[dpcmoff++]=val;
					dpcmsize++;
					size++;
					j=0;
				}
				off++;
			}

			dpcmList[i].size=size>>4;
			dpcmoff=((dpcmoff>>6)+1)<<6;
		}
	}

	setptr(0);

	if(find("[DMC0]",textSize,0)>=0)
	{
		find("SamplesAssigned=",textSize,1);

		for(i=0;i<96;i++)
		{
			val=readnum();

			if(i>=3*12&&i<4*12&&val)
			{
				dpcmInsList[i-3*12].off=dpcmList[val-1].off;
				dpcmInsList[i-3*12].size=dpcmList[val-1].size;
			}

			textPtr++;
		}

		find("SamplesPitch=",textSize,1);

		for(i=0;i<96;i++)
		{
			val=readnum();

			if(i>=3*12&&i<4*12)
			{
				dpcmInsList[i-3*12].pitch=val;
			}

			textPtr++;
		}

		find("SamplesLoop=",textSize,1);

		for(i=0;i<96;i++)
		{
			val=readnum();

			if(i>=3*12&&i<4*12)
			{
				dpcmInsList[i-3*12].loop=val;
			}

			textPtr++;
		}
	}

	free(text);

	//remove redundant instrument numbers

	for(i=0;i<5;i++)
	{
		prev=-1;
		for(j=0;j<rowcnt;j++)
		{
			if(rowList[j]==loop) prev=-1;//always keep instrument on the loop position to have correct instrument after loop
			if(chnList[i][j].ins==prev||i==4)//no instruments on dpcm channel
			{
				chnList[i][j].ins=0;
			}
			else
			{
				if(chnList[i][j].ins) prev=chnList[i][j].ins;
			}
		}
	}

	//create an assembly text with data

	strcpy(songname,argv[1]);
	songname[strlen(songname)-4]=0;
	for(i=strlen(songname)-1;i>=0;i--)
	{
		if(songname[i]=='\\'||songname[i]=='/')
		{
			strcpy(songname,&songname[i+1]);
			break;
		}
	}

	outsize=0;
	inssize=0;

	strcpy(name,argv[1]);
	name[strlen(name)-4]=0;
	strcat(name,outtype==OUT_CA65?".s":".asm");

	file=fopen(name,"wt");

	if(!file)
	{
		printf("Can't create file\n");
		return 1;
	}

	if(!outinsonly)
	{
		//module label

		fprintf(file,"%s_module:\n",songname);

		//pointers to channels data, instruments

		if(outins)
		{
			fprintf(file,"\t%s %cchn0,%cchn1,%cchn2,%cchn3,%cchn4,%cins\n",dw,local,local,local,local,local,local);
		}
		else
		{
			fprintf(file,"\t%s %cchn0,%cchn1,%cchn2,%cchn3,%cchn4,music_instruments\n",dw,local,local,local,local,local);
		}

		outsize+=6*2;

		//global speed

		fprintf(file,"\t%s $%2.2x\n",db,speed);
		outsize++;
	}
	else
	{
		fprintf(file,"music_instruments:\n");
	}

	if(outins||outinsonly)
	{
		//instruments

		fprintf(file,"%cins:\n\t%s %cenv_default,%cenv_default,%cenv_default\n",local,dw,local,local,local);
		fprintf(file,"\t%s $30,$00\n",db);

		outsize+=3*2+2;

		for(i=0;i<64;i++)
		{
			if(insList[i].number)
			{
				fprintf(file,"\t%s ",dw);

				if(insList[i].envVolume  !=0x00ffffff&&envList[insList[i].envVolume  +0  ].len>0) fprintf(file,"%cenv_vol%i," ,local,insList[i].envVolume);   else fprintf(file,"%cenv_default,",local);
				if(insList[i].envArpeggio!=0x00ffffff&&envList[insList[i].envArpeggio+128].len>0) fprintf(file,"%cenv_arp%i," ,local,insList[i].envArpeggio); else fprintf(file,"%cenv_default,",local);
				if(insList[i].envPitch   !=0x00ffffff&&envList[insList[i].envPitch   +256].len>0) fprintf(file,"%cenv_pitch%i",local,insList[i].envPitch);    else fprintf(file,"%cenv_default" ,local);

				fprintf(file,"\n\t%s $%2.2x,$00\n",db,insList[i].duty);

				outsize+=3*2+2;
			}
		}

		//default envelope

		fprintf(file,"%cenv_default:\n\t%s $c0,$7f,$00\n",local,db);
		outsize+=3;

		//envelopes

		for(i=0;i<128*3;i++)
		{
			if(envList[i].len)
			{
				switch(i/128)
				{
				case 0: sprintf(str,"vol"); break;
				case 1: sprintf(str,"arp"); break;
				case 2: sprintf(str,"pitch"); break;
				}

				fprintf(file,"%cenv_%s%i:\n",local,str,i&127);

				len=1;
				prev=-1;
				cleardata();
				off=-1;

				for(j=0;j<envList[i].len;j++)
				{
					if(envList[i].value[j]!=prev)
					{
						if(len>1) putdata(len==2?prev+192:len-2);
						if(envList[i].loop==j) off=dataPtr;
						putdata(envList[i].value[j]+192);

						len=1;
						prev=envList[i].value[j];
					}
					else
					{
						len++;
					}
				}

				if(len>1) putdata(len==2?prev+192:len-2);
				if(off<0) off=dataPtr-1;
				putdata(127);
				putdata(off);
				outsize+=dataPtr;
				fdumpdata(file);
			}
		}

		inssize=outsize-6*2-1;
	}

	if(!outinsonly)
	{
		//channels

		for(i=0;i<5;i++)
		{
			fprintf(file,"\n%cchn%i:\n",local,i);

			for(j=0;j<rowcnt/blocksize;j++)
			{
				cleardata();
				ref=-1;

				for(k=j*blocksize;k<(j+1)*blocksize;k++)
				{
					if(rowList[k]==loop)
					{
						fprintf(file,"%cchn%i_loop:\n",local,i);
						break;
					}
				}

				fprintf(file,"%cchn%i_%i:\n",local,i,j);

				for(k=0;k<j;k++)
				{
					if(!memcmp(&chnList[i][j*blocksize],&chnList[i][k*blocksize],4*blocksize))
					{
						ref=k;
						break;
					}
				}

				if(ref>=0)
				{
					len=0;

					for(k=ref*blocksize;k<(ref+1)*blocksize;k++)
					{
						if(chnList[i][k].note!=255) len++;
						if(chnList[i][k].ins||chnList[i][k].fx||chnList[i][k].param) len++;
					}

					if(!len)
					{
						fprintf(file,"\t%s $%2.2x\n",db,127+blocksize);
						outsize++;
					}
					else
					{
						fprintf(file,"\t%s $ff,$%2.2x\n\t%s %cchn%i_%i\n",db,blocksize,dw,local,i,ref);
						outsize+=4;
					}
				}
				else
				{
					len=0;

					for(k=j*blocksize;k<(j+1)*blocksize;k++)
					{
						if(chnList[i][k].fx=='F')
						{
							if(chnList[i][k].param>0&&chnList[i][k].param<0x1a)
							{
								if(len) putdata(127+len);
								putdata(chnList[i][k].param+192);
								len=0;
							}
						}
						if(chnList[i][k].note!=255)
						{
							if(len) putdata(127+len);
							if(chnList[i][k].ins&&i<4) putdata(chnList[i][k].ins+64);
							putdata(chnList[i][k].note);
							len=0;
						}
						else
						{
							len++;
						}
					}

					if(len) putdata(127+len);
				}

				outsize+=dataPtr;
				fdumpdata(file);
			}

			cleardata();
			putdata(254);
			outsize+=dataPtr;
			fdumpdata(file);

			fprintf(file,"\t%s %cchn%i_loop\n",dw,local,i);
			outsize+=2;
		}
	}

	fclose(file);

	if(!outinsonly)
	{
		printf("Binary music data total size %i bytes\n",outsize);
	}
	else
	{
		printf("Music data export disabled\n");
	}

	if(outins)
	{
		printf("Instruments data total size %i bytes\n",inssize);
	}
	else
	{
		printf("Instruments data export disabled\n");
	}

	if(dpcmoff&&outdpcm&&!outinsonly)
	{
		strcpy(name,argv[1]);
		name[strlen(name)-4]=0;
		strcat(name,"_dpcm.bin");

		file=fopen(name,"wb");
		fwrite(dpcm,dpcmoff,1,file);
		fclose(file);

		strcpy(name,argv[1]);
		name[strlen(name)-4]=0;
		strcat(name,"_dpcm");
		strcat(name,outtype==OUT_CA65?".s":".asm");

		file=fopen(name,"wt");
		fprintf(file,"%s_dpcm:\n",songname);

		for(i=0;i<12;i++)
		{
			fprintf(file,"\t%s $%2.2x+FT_DPCM_PTR,$%2.2x,$%2.2x,$00\n",db,dpcmInsList[i].off,dpcmInsList[i].size,dpcmInsList[i].pitch|((dpcmInsList[i].loop&1)<<6));
		}

		fclose(file);

		printf("Binary DPCM data size %i bytes\n",dpcmoff);
	}
	else
	{
		printf(outdpcm?"No DPCM data\n":"DPCM export disabled\n");
	}

	return 0;
}

