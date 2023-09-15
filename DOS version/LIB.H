/*
LIB.H
Provides all the functions to load bitmaps from the ressurces of the game Historyline 1914-1918.
DOS Version 1.0 - 20.4.2023
by Jan Knipperts
*/

const int Num_Parts = 175; //Number of HL terrain parts
const int Num_Units = 51; // Number of HL Units

struct Index_Entry
{
	char			RES_Name[8];
	dword			Offset;
};

struct Bitmap_Header
{
	char			id[8];
	byte			transparent;
	byte			type;
	word			pad_word1;  //Seems to be always 0
	word			pad_word2;  //Seems to be always 0
	word			width;
	word			height;
};

struct Partlib_Infos
{
	dword			data_size;
	byte huge*		data;
	dword 			index_offset;
	Index_Entry		Index[Num_Parts];
};

struct Unitlib_Infos
{
	dword			data_size;
	byte huge*		data;
	dword 			index_offset;
	Index_Entry		Index[Num_Units*6];//6 bitmaps of unit in different angels
};

struct TPWM_Infos
{
	dword			unpacked_size;
	dword			packed_size;
	byte huge*		packed_data;
	byte huge*		unpacked_data;
};

struct Partdat_Infos
{
	char			name[Num_Parts][8];
};

struct Unitdat_Infos
{
	char			name[Num_Units*6][8]; //6 bitmaps of unit in different angels
};



Partlib_Infos		Partlib;
Unitlib_Infos		Unitlib;
Partdat_Infos		Partdat;
Unitdat_Infos		Unitdat;
TPWM_Infos		TPWM;
dword			ID;


dword			bitmap_offset;
Bitmap_Header		BM_Header;
byte			*pal;






int getbit(char n, int bitwanted)
// Returns the state of a bit
{
	int thebit = (n & (1 << bitwanted)) >> bitwanted;
	return thebit;
}




int TPWM_Unpack()
// Unpacks data that is compressed with Turbo Packer
// TPWM struct musst be set
{
	unsigned char b1;
	unsigned char b2;
	unsigned char packbyte;
	char	      bit;
	unsigned int  i;
	unsigned long inofs;
	unsigned long outofs;
	unsigned int  distance;
	unsigned int  length;


	outofs = 0;
	inofs = 0;


	while (outofs < TPWM.unpacked_size)
	{
		packbyte = TPWM.packed_data[inofs];

		inofs++;

		for (bit = 7; bit >= 0; bit--)
		{
			if (getbit(packbyte, bit) > 0)
			{
				b1 = TPWM.packed_data[inofs];
				inofs++;
				b2 = TPWM.packed_data[inofs];
				inofs++;

				distance = (unsigned int)((b1 & 0xF0) << 4) | b2;
				length = (unsigned int)(b1 & 0x0F) + 3;


				for (i = 0; i <= (length - 1); i++)
				{
					TPWM.unpacked_data[outofs] = TPWM.unpacked_data[(outofs - distance)];
					outofs++;
					if (outofs >= TPWM.unpacked_size) break;
				}
			}
			else
			{
				TPWM.unpacked_data[outofs] = TPWM.packed_data[inofs];
				inofs++;
				outofs++;
			}
		}
	}

	if (outofs >= TPWM.unpacked_size)
		return 0;
	else
		return -1;
}




int Read_Chunk(FILE* f,dword size,byte huge* dest)
{
	size_t 				IO_result;
	byte*				IO_buffer;
	dword				IO_offset;

	IO_buffer = (unsigned char*) malloc(IO_BUFFER_SIZE); //4K IO Buffer
	if (IO_buffer == NULL)
	{
		fclose(f);
		return -4; //Reserve memory buffer failed
	}

	memset (IO_buffer,0,IO_BUFFER_SIZE);
	IO_offset = 0;

	while (IO_offset+IO_BUFFER_SIZE < size)   //Data
	{
	  IO_result = fread(IO_buffer,IO_BUFFER_SIZE,1,f);
	  if (IO_result != 1) //Read Error?
	  {
		free(IO_buffer);
		fclose(f);
		return -2;
	  }
	  _fmemcpy((byte huge*)dest+IO_offset,MK_FP(FP_SEG(IO_buffer),FP_OFF(IO_buffer)),IO_BUFFER_SIZE);
	  memset(IO_buffer,0,IO_BUFFER_SIZE);
	  IO_offset = IO_offset + IO_BUFFER_SIZE;
	}
	  IO_result = fread(IO_buffer,size-IO_offset,1,f);
	  if (IO_result != 1) //Read Error?
	  {
		free(IO_buffer);
		fclose(f);
		return -2;
	  }
	  _fmemcpy((byte huge*)dest+IO_offset,MK_FP(FP_SEG(IO_buffer),FP_OFF(IO_buffer)),size-IO_offset);

	free(IO_buffer);
	return 0;
}


int Unpack_file(FILE* f)
// Unpacks a file compressed with Turbo Packer
{
	size_t 				IO_result;
	int				result;

	fseek(f, 0, SEEK_END);	//Get file size
	TPWM.packed_size = ftell(f) - 8;
	rewind(f);
	fseek(f, 4, SEEK_SET);

	IO_result = fread(&TPWM.unpacked_size, sizeof(TPWM.unpacked_size), 1, f); //Read unpacked size

	if (IO_result != 1) //Read Error?
	{
		fclose(f);
		return -2;
	}
	TPWM.packed_data = (byte huge*)farmalloc(TPWM.packed_size);

	if (TPWM.packed_data == NULL)
	{
		fclose(f);
		return -4; //Reserve memory buffer for packed data failed
	}

	_fmemset((byte huge*)TPWM.packed_data, 0, TPWM.packed_size);  //Clear it;

	if (Read_Chunk(f,TPWM.packed_size,TPWM.packed_data) != 0)
	{
		farfree(TPWM.packed_data);
		fclose(f);
		return -2;
	}

	fclose(f);

	TPWM.unpacked_data = (byte huge*) farmalloc(TPWM.unpacked_size);

	if (TPWM.unpacked_data == NULL) //Reserve memory buffer for unpacked data
	{
		farfree(TPWM.packed_data);
		return -4;
	}
	_fmemset((byte huge*) TPWM.unpacked_data, 0, TPWM.unpacked_size);  //Clear it;


	if (TPWM_Unpack() != 0)
	{
		farfree(TPWM.packed_data);
		farfree(TPWM.unpacked_data);
		return -5;	//Decompression failed
	}

	farfree(TPWM.packed_data); //Free buffer for packed data - we don't need it anymore
	return 0;
}



int Load_Palette(char* pal_filename)
// Loads a Palette file from the game
{
	FILE* 		pal_file;
	size_t		IO_result;
	byte		r, g, b;
	int		i,i1,o;
	dword		fsize;

	pal_file = fopen(pal_filename, "rb");

	if (!pal_file)
	{
		return -1;
	}


	IO_result = fread(&ID, sizeof(ID), 1, pal_file); //Read first four byte

	if (IO_result != 1)
	{
		fclose(pal_file);
		return -2; //Read Error
	}

	fseek(pal_file, 0, SEEK_END);	//Get file size
	fsize = ftell(pal_file);
	rewind(pal_file);

	if (ID == 0x4D575054) //"TPWM" string?
	{

		if (Unpack_file(pal_file) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		if (TPWM.unpacked_size < 256 * 3)
		{
			farfree(TPWM.unpacked_data);
			return -3;		//File is corrupted or no palette file
		}

		pal = (byte*)malloc(TPWM.unpacked_size);
		_fmemcpy(MK_FP(FP_SEG(pal),FP_OFF(pal)),(byte huge*)TPWM.unpacked_data,TPWM.unpacked_size);
		farfree(TPWM.unpacked_data);
	}
	else
	{
		if (fsize < 256 * 3)
		{
			fclose(pal_file);
			return -3;
		}

		pal = (byte*)malloc(fsize);
		IO_result = fread(pal,fsize,1,pal_file);  //Read palette data
		if (IO_result != 1)
		{
			fclose(pal_file);
			return -2; //Read Error
		}


		fclose(pal_file);
	}

	return 0;
}



int Load_Part_files(char* partlib_filename, char* partdat_filename)
// Loads all terrain and building data into memory (PARTS.LIB and PARTS.DAT)
{
	FILE* 				pf;
	size_t 				IO_result;
	dword				fsize;
	byte huge*			Pointer_Index;
	dword				a,b;


	pf = fopen(partlib_filename, "rb");

	if (!pf)
	{
	  return -1;
	}

	fseek(pf, 0, SEEK_END);	//Get file size
	fsize = ftell(pf);
	rewind(pf);


	IO_result = fread(&ID, sizeof(ID), 1, pf); //Read ID

	if (IO_result != 1)
	{
		fclose(pf);
		return -2; //Read Error
	}


	if (ID == 0x4D575054) //"TPWM" string?
	{
		//Yes, so file is TPWM packed

		if (Unpack_file(pf) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}


		//Get index offset

		Partlib.index_offset =  (unsigned long) (TPWM.unpacked_data[0]); //0
		Partlib.index_offset |= ((unsigned long) TPWM.unpacked_data[1]) << 8;
		Partlib.index_offset |= ((unsigned long) TPWM.unpacked_data[2]) << 16;
		Partlib.index_offset |= ((unsigned long) TPWM.unpacked_data[3]) << 24;


		//Check file integrity

		if (Partlib.index_offset >= TPWM.unpacked_size)
		{
			farfree(TPWM.unpacked_data);
			return -3;
		}
		if ((TPWM.unpacked_size - Partlib.index_offset) != sizeof(Partlib.Index))
		{
			farfree(TPWM.unpacked_data);
			return -3;
		}

		Partlib.data_size = Partlib.index_offset - 4;

		Partlib.data = TPWM.unpacked_data+4; //Pointer to data area
		_fmemcpy(MK_FP(FP_SEG(Partlib.Index),FP_OFF(Partlib.Index)),(byte huge*) TPWM.unpacked_data + Partlib.index_offset,TPWM.unpacked_size - Partlib.index_offset);

	}
	else
	{
		// File is uncompressed

		Partlib.index_offset = ID;


		if (Partlib.index_offset > fsize)
		{
			fclose(pf);
			return -3;
		}


		if ((fsize - Partlib.index_offset) != (Num_Parts * 12))
		{
			fclose(pf);
			return -3;
		}


		Partlib.data_size = Partlib.index_offset - 4;
		Partlib.data = (byte huge*)farmalloc(fsize);

		if (Partlib.data == NULL)
		{
			fclose(pf);
			return -4; //Reserve memory buffer for bitmap data failed
		}

		_fmemset((byte huge*)Partlib.data, 0, Partlib.data_size);  //Clear it;


		if (Read_Chunk(pf,Partlib.data_size,Partlib.data) != 0)
		{
			fclose(pf);
			return -2;
		}


		if (Read_Chunk(pf,(fsize - Partlib.index_offset),(byte huge*) MK_FP(FP_SEG(Partlib.Index),FP_OFF(Partlib.Index))) != 0)
		{
			fclose(pf);
			return -2;
		}


		fclose(pf);
	}


	//====== Parts.dat

	pf = fopen(partdat_filename, "rb");
	if (!pf)
	{
		return -1;
	}

	IO_result = fread(&ID, sizeof(ID), 1, pf);
	if (IO_result != 1)
	{
		fclose(pf);
		return -2; //Read Error
	}

	fseek(pf, 0, SEEK_END); //Get file size and set file pointer to 0 again
	fsize = ftell(pf);
	rewind(pf);

	if (ID == 0x4D575054) // "TPWM" String?
	{
		//File is packed
		if (Unpack_file(pf) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		if (TPWM.unpacked_size != (Num_Parts * 8))
		{
			farfree(TPWM.unpacked_data);
			return -3; //File is corrupted
		}

		_fmemcpy(MK_FP(FP_SEG(Partdat.name),FP_OFF(Partdat.name)),(byte huge*)TPWM.unpacked_data, TPWM.unpacked_size);

		farfree(TPWM.unpacked_data); //Free buffer for packed data - we don't need it anymore
	}
	else
	{
		//File is unpacked


		if (fsize != (Num_Parts * 8))
		{
			fclose(pf);
			return -3; //File is corrupted
		}

		IO_result = fread(&Partdat.name, sizeof(Partdat.name), 1, pf);
		if (IO_result != 1)
		{
			fclose(pf);
			return -2; //Read Error
		}

		fclose(pf);
	}	

	return 0;
}





int Load_Unit_files(char* unitlib_filename, char* unitdat_filename)
//Load all unit data into memory (UNIT.LIB and UNIT.DAT)
{
	FILE*				uf;
	size_t 				IO_result;
	dword				fsize;



	uf = fopen(unitlib_filename, "rb");

	if (!uf)
	{
		return -1;
	}

	fseek(uf, 0, SEEK_END);	//Get file size
	fsize = ftell(uf);
	rewind(uf);


	IO_result = fread(&ID, sizeof(ID), 1, uf); //Read ID

	if (IO_result != 1)
	{
		fclose(uf);
		return -2; //Read Error
	}


	if (ID == 0x4D575054) //"TPWM" string?
	{
			//Yes, so file is TPWM packed

		if (Unpack_file(uf) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		//Get index offset

		Unitlib.index_offset =  (unsigned long) (TPWM.unpacked_data[0]); //0
		Unitlib.index_offset |= ((unsigned long) TPWM.unpacked_data[1]) << 8;
		Unitlib.index_offset |= ((unsigned long) TPWM.unpacked_data[2]) << 16;
		Unitlib.index_offset |= ((unsigned long) TPWM.unpacked_data[3]) << 24;

		//Check file integrity

		if (Unitlib.index_offset >= TPWM.unpacked_size)
		{
			farfree(TPWM.unpacked_data);
			return -3;
		}
		if ((TPWM.unpacked_size - Unitlib.index_offset) != sizeof(Unitlib.Index))
		{
			farfree(TPWM.unpacked_data);
			return -3;
		}

		Unitlib.data_size = Unitlib.index_offset - 4;
		Unitlib.data = TPWM.unpacked_data+4; //Pointer to data area
		_fmemcpy(MK_FP(FP_SEG(Unitlib.Index),FP_OFF(Unitlib.Index)),(byte huge*)TPWM.unpacked_data + Unitlib.index_offset,TPWM.unpacked_size - Unitlib.index_offset);
	}
	else
	{
		// File is uncompressed

		Unitlib.index_offset = ID;


		if (Unitlib.index_offset > fsize)
		{
			fclose(uf);
			return -3;
		}


		if ((fsize - Unitlib.index_offset) != (Num_Units * 12))
		{
			fclose(uf);
			return -3;
		}


		Unitlib.data_size = Unitlib.index_offset - 4;
		Unitlib.data = (byte huge*)farmalloc(fsize);

		if (Unitlib.data == NULL)
		{
			fclose(uf);
			return -4; //Reserve memory buffer for bitmap data failed
		}

		_fmemset((byte huge*)Unitlib.data, 0, Unitlib.data_size);  //Clear it;


		if (Read_Chunk(uf,Unitlib.data_size,Unitlib.data) != 0)
		{
			fclose(uf);
			return -2;
		}

		IO_result = fread(&Unitlib.Index,sizeof(Unitlib.Index), 1, uf); //Read Index

		if (IO_result != 1)
		{
			fclose(uf);
			return -2; //Read Error
		}


		fclose(uf);
	}


	//====== Unit.dat

	uf = fopen(unitdat_filename, "rb");
	if (!uf)
	{
		return -1;
	}


	IO_result = fread(&ID, sizeof(ID), 1, uf);
	if (IO_result != 1)
	{
		fclose(uf);
		return -2; //Read Error
	}

	fseek(uf, 0, SEEK_END); //Get file size and set file pointer to 0 again
	fsize = ftell(uf);
	rewind(uf);

	if (ID == 0x4D575054) // "TPWM" String?
	{

		//File is packed
		if (Unpack_file(uf) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		if (TPWM.unpacked_size != sizeof(Unitdat.name))
		{
			farfree(TPWM.unpacked_data);
			return -3; //File is corrupted
		}
		_fmemcpy(MK_FP(FP_SEG(Unitdat.name),FP_OFF(Unitdat.name)),(byte huge*)TPWM.unpacked_data, TPWM.unpacked_size);

		farfree(TPWM.unpacked_data); //Free buffer for packed data - we don't need it anymore
	}
	else
	{
		//File is unpacked


		if (fsize != (Num_Units * 8))
		{
			fclose(uf);
			return -3; //File is corrupted
		}

		IO_result = fread(&Unitdat.name, sizeof(Unitdat.name), 1, uf);
		if (IO_result != 1)
		{
			fclose(uf);
			return -2; //Read Error
		}

		fclose(uf);
	}

	return 0;
}



void Get_8BPP(byte far * Bitmap, int pos_x, int pos_y)
{
  int start_x = 0;
  int blocks = 0;
  int p = 0;
  int x = 0;
  int y = 0;
  int i = 0;
  byte color = 0;



  blocks = BM_Header.width / 4;
  if ((BM_Header.width % 4) != 0) ++blocks;


  for (p = 0; p < 4; ++p)
  {

      for (y = 0; y < BM_Header.height; ++y)
      {

	  for (i = 0; i < blocks; ++i)
	  {
	      x = i * 4 + start_x;
	      if (x <= BM_Header.width - 1)
	      {
			  color = Partlib.data[bitmap_offset];  //Get next byte
			  ++bitmap_offset;
			  if (color != BM_Header.transparent)
			   Bitmap[((pos_y + y)*SCREEN_WIDTH) + (pos_x + x)] = color;

	      }
	  }

      }
      ++start_x;

   }


  return;
}




void Get_4BPP(byte far* Bitmap,int pos_x, int pos_y, int side)
{
  int start_x = 0;
  int blocks = 0;
  int p = 0;
  int y = 0;
  int x = 0;
  int i = 0;
  byte color = 0;
  byte color1 = 0;
  byte color2 = 0;

  blocks = BM_Header.width / 4;
  if ((BM_Header.width % 4) != 0) ++blocks;

  if (side == 1) side = 32; // colors for german units
  if (side == 2) side = 48; // colors for french units
  if (side == 3) side = 192; // neutral

  for (p = 0; p < 4; ++p)
  {
      for (y = 0; y < BM_Header.height; ++y)
      {
		for (i = 0; i < blocks; ++i)
		{
	      x = i * 4 + start_x;

		  if (x <= BM_Header.width - 1)
	      {
			if (i % 2 == 0) //is i even?
			{
				color = Unitlib.data[bitmap_offset];  //Get next byte
				++bitmap_offset;

				color1 = (byte) (color  >> 4);
				color2 = (byte) (color & 0x0F);
				if (color1 != BM_Header.transparent)
				Bitmap[((pos_y + y)*SCREEN_WIDTH) + (pos_x + x)] = color1+side;
			}
			else
				if (color2 != BM_Header.transparent)
				Bitmap[((pos_y + y)*SCREEN_WIDTH) + (pos_x + x)] = color2+side;

	      }
		}
      }
      ++start_x;
   }

  return;
}



int Translate_Partnum(int part_num)
//In HL14-18, the order of the bitmaps in the library does not match the numbers of the bitmaps in the map data.
//The correct order is in a dat file and this function takes over the translation
{
	int		i;
	int		o;
	char		s1[9];
	char		s2[9];


	memcpy(s1,Partdat.name[part_num],8);
	s1[8] = (char) 0;

	o = -1;

	for (i = 0; i <= Num_Parts; ++i)
	{
		memcpy(s2,Partlib.Index[i].RES_Name,8);
		s2[8] = (char) 0;

		if (strcmpi(s1,s2) == 0)
		{
			o = i;
			break;
		}
	}

	if (o == -1) o = Num_Parts; //Part bitmap not found -> draw "Verboten"
	return o;
}


int Translate_Unitnum(int unit_num)
//In HL14-18, the order of the bitmaps in the library does not match the numbers of the bitmaps in the map data.
//The correct order is in a dat file and this function takes over the translation
{
	int		i;
	int		o;
	char		s1[9];
	char		s2[9];


	memcpy(s1,Unitdat.name[unit_num],8);
	s1[8] = (char) 0;

	o = -1;

	for (i = 0; i <= (Num_Units*6); ++i)
	{
		memcpy(s2,Unitlib.Index[i].RES_Name,8);
		s2[8] = (char) 0;

		if (strcmpi(s1,s2) == 0)
		{
			o = i;
			break;
		}
	}

	return o;
}



int Draw_Part(byte far* Bitmap,int xpos, int ypos, int part_num)
{
	if (part_num > Num_Parts)
	  part_num = Num_Parts; //Invalid Part number -> draw "Verboten"
	else
	{
	  part_num = Translate_Partnum(part_num);
	  if (part_num > Num_Parts) part_num = Num_Parts; //Invalid Part number -> draw "Verboten"
	}

	bitmap_offset = Partlib.Index[part_num].Offset-4;
	if (bitmap_offset > (Partlib.data_size - sizeof(BM_Header))) return -1;

	_fmemcpy(MK_FP(FP_SEG(BM_Header.id),FP_OFF(BM_Header.id)),(byte huge*) Partlib.data+bitmap_offset,sizeof(BM_Header));

	bitmap_offset = bitmap_offset + sizeof(BM_Header);
	if ((bitmap_offset + (BM_Header.width * BM_Header.height)) > Partlib.data_size) return -1;

	Get_8BPP(Bitmap,xpos, ypos);
	return 0;
}

int Draw_Unit(byte far* Bitmap, int xpos, int ypos, int unit_num, int side)
{
	if (unit_num > Num_Units*6) return -1;

	unit_num = Translate_Unitnum(unit_num);
	if (unit_num == -1) return -1;

	if (unit_num > Num_Units*6) return -1;

	bitmap_offset = Unitlib.Index[unit_num].Offset - 4;
	if (bitmap_offset > (Unitlib.data_size - sizeof(BM_Header))) return -1;

	_fmemcpy(MK_FP(FP_SEG(BM_Header.id),FP_OFF(BM_Header.id)),(byte huge*) Unitlib.data+bitmap_offset,sizeof(BM_Header));

	bitmap_offset = bitmap_offset + sizeof(BM_Header);
	if ((bitmap_offset + (BM_Header.width * BM_Header.height)) > Unitlib.data_size) return -1;

	Get_4BPP(Bitmap,xpos, ypos,side);
	return 0;
}












