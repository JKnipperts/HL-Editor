
/*
LIB.H by Jan Knipperts
Provides all the functions to load bitmaps from the ressurce files of the game Historyline 1914-1918.
Version 1.3 - 11.12.2023
- Fixed a bug in TPWM_unpack that caused writing outside the buffer in some cases.

The main features:

TPWM_Unpack / Unpack_file
Unpacks data that is compressed with Turbo Packer

Load_Palette
Loads Hisotry Line 1914-1918 palette files to "HL_Palette"

Load_Part_files
Loads all terrain and building data from the game files into memory

Load_Unit_files
Loads all unit data from game files into memory

Draw_Part
Draws the given unit to a QImage named "MapImage" that is used as screen buffer

Draw_Unit
Draws the given unit to a QImage named "MapImage" that is used as screen buffer


*/


#include <iostream>
#include <string>
#include <sstream>
#include <QImage>

const int Num_Parts = 175; //Number of HL terrain parts
const int Num_Units = 51; // Number of HL Units

struct Index_Entry
{
    char                    RES_Name[8];
    unsigned int            Offset;
};

struct Bitmap_Header
{
    char                    id[8];
    unsigned char			transparent;
    unsigned char			type;
    unsigned short int		pad_word1;  //Seems to be always 0
    unsigned short int		pad_word2;  //Seems to be always 0
    unsigned short int		width;
    unsigned short int		height;
};

struct Palette_Entry
{
    unsigned char           Red;
    unsigned char           Green;
    unsigned char           Blue;
};



struct Partlib_Infos
{
    unsigned int			data_size;
    unsigned char*			data;
    unsigned int 			index_offset;
    Index_Entry             Index[Num_Parts];
};

struct Unitlib_Infos
{
    unsigned int			data_size;
    unsigned char*			data;
    unsigned int 			index_offset;
    Index_Entry             Index[Num_Units*6];//6 bitmaps of unit in different angels
};

struct TPWM_Infos
{
    unsigned int			unpacked_size;
    unsigned int			packed_size;
    unsigned char*			packed_data;
    unsigned char*			unpacked_data;
};

struct Partdat_Infos
{
    char                    name[Num_Parts][8];
};

struct Unitdat_Infos
{
    char                    name[Num_Units*6][8]; //6 bitmaps of unit in different angels
};



Partlib_Infos               Partlib;
Unitlib_Infos               Unitlib;
Partdat_Infos               Partdat;
Unitdat_Infos               Unitdat;
TPWM_Infos                  TPWM;
unsigned int				ID;


unsigned int				bitmap_offset;
Bitmap_Header               BM_Header;
Palette_Entry               HL_Palette[256];



std::string int2string(int d) // int zu string
{

    std::string s;


    std::stringstream str;

	str << d;
	str >> s;
	return s;
}

// converts character array
// to string and returns it


std::string char2string(char* a, int size)
{
	int i;

    std::string s = "";
	for (i = 0; i < size; i++) {
		s = s + a[i];
	}
	return s;
}


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
	unsigned int  inofs;
	unsigned int  outofs;
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
            if (getbit(packbyte, bit) == 1)
			{
				b1 = TPWM.packed_data[inofs];
				inofs++;
				b2 = TPWM.packed_data[inofs];
				inofs++;

				distance = (unsigned int)((b1 & 0xF0) << 4) | b2;
                if (distance > outofs) break;
				length = (unsigned int)(b1 & 0x0F) + 3;

				for (i = 0; i <= (length - 1); i++)
                {
                    if (outofs < TPWM.unpacked_size)
                    {
                        TPWM.unpacked_data[outofs] = TPWM.unpacked_data[(outofs - distance)];
                        outofs++;
                    }
                    else
                        break;
				}
			}
			else
            {
                if ((outofs < TPWM.unpacked_size) && (inofs < TPWM.packed_size))
                {
                  TPWM.unpacked_data[outofs] = TPWM.packed_data[inofs];
                  inofs++;
                  outofs++;
                }
                else
                  break;

			}
        }

        if (outofs >= TPWM.unpacked_size)
            break;

    }




    if (outofs == TPWM.unpacked_size)
		return 0;
	else
		return -1;
}



int Unpack_file(FILE* f)
// Unpacks a file compressed with Turbo Packer
{
	size_t 				IO_result;


    if (!f) return -1;

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


    if ((TPWM.packed_data = (unsigned char*)malloc(TPWM.packed_size+1)) == NULL) //Reserve memory buffer for packed data
	{
		fclose(f);
        return -4;
	}


    memset(TPWM.packed_data, 0, TPWM.packed_size+1);  //Clear it;

	IO_result = fread(TPWM.packed_data, TPWM.packed_size, 1, f); //Read packed dara

	if (IO_result != 1) //Read Error?
	{
		free(TPWM.packed_data);
		fclose(f);
		return -2;
	}
	
    fclose(f);

    if ((TPWM.unpacked_data = (unsigned char*)malloc(TPWM.unpacked_size+1)) == NULL) //Reserve memory buffer for unpacked data
	{
		free(TPWM.packed_data);
		return -4;
	}

    memset(TPWM.unpacked_data, 0, TPWM.unpacked_size);  //Clear it;



	if (TPWM_Unpack() != 0)		
    {
        free(TPWM.packed_data);
        free(TPWM.unpacked_data);

        return -5;	//Decompression failed
	}


	free(TPWM.packed_data); //Free buffer for packed data - we don't need it anymore
	return 0;
}



int Load_Palette(std::string pal_filename)
// Loads a Palette file from the game
{
    FILE* f;
    size_t          IO_result;
    int             i,o;
    unsigned int	fsize;

    fopen_s(&f, pal_filename.data(), "rb");

    if (!f)
    {
        return -1;
    }


    IO_result = fread(&ID, sizeof(ID), 1, f); //Read ID

    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Read Error
    }

    fseek(f, 0, SEEK_END);	//Get file size
    fsize = ftell(f);


    if (ID == 0x4D575054) //"TPWM" string?
    {
        if (Unpack_file(f) != 0)
        {
            free(TPWM.unpacked_data);
            return -5;		//Decompressing failed!
        }

        if (TPWM.unpacked_size < 256 * 3)
        {
            free(TPWM.unpacked_data);
            return -3;		//File is corrupted or no palette file
        }

        o = 0;
        for (i = 0; i < 256; i++)
        {
            HL_Palette[i].Red = TPWM.unpacked_data[o];
            HL_Palette[i].Green = TPWM.unpacked_data[o + 1];
            HL_Palette[i].Blue = TPWM.unpacked_data[o + 2];
            o = o + 3;
        }
    }
	else
	{

		if (fsize < 256 * 3)
		{
			fclose(f);
			return -3;
		}

        rewind(f);

		for (i = 0; i < 256; i++)
		{
            fread(&HL_Palette[i].Red, 1, 1, f);  //Read red value
            fread(&HL_Palette[i].Green, 1, 1, f);  //Read green value
            fread(&HL_Palette[i].Blue, 1, 1, f);  //Read blue value
		}
		fclose(f);
	}

	return 0;
}



int Load_Part_files(std::string partlib_filename,std::string partdat_filename)
// Loads all terrain and building data into memory (PARTS.LIB and PARTS.DAT)
{
    FILE*                       f;
    size_t                      IO_result;
    unsigned int				fsize;

    fopen_s(&f, partlib_filename.data(), "rb");


	if (!f)
	{
		return -1;
	}

	fseek(f, 0, SEEK_END);	//Get file size
	fsize = ftell(f);
	rewind(f);


	IO_result = fread(&ID, sizeof(ID), 1, f); //Read ID

	if (IO_result != 1)
	{
		fclose(f);
		return -2; //Read Error
	}


	if (ID == 0x4D575054) //"TPWM" string?
	{
		//Yes, so file is TPWM packed

        if (Unpack_file(f) != 0)
		{
			free(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
        }

        memcpy(&Partlib.index_offset, TPWM.unpacked_data, sizeof(Partlib.index_offset)); //Get index offset

		//Check file integrity

		if (Partlib.index_offset >= TPWM.unpacked_size)
		{
			free(TPWM.unpacked_data);
			return -3;
		}
		if ((TPWM.unpacked_size - Partlib.index_offset) != sizeof(Partlib.Index))
		{
			free(TPWM.unpacked_data);
			return -3;
		}

		Partlib.data_size = Partlib.index_offset - 4;
        if ((Partlib.data = (unsigned char*)malloc(Partlib.data_size)) == NULL)
		{
			free(TPWM.unpacked_data); //free buffer for unpacked data	
			return -4; //Create buffer for bitmap data
		}
		memset(Partlib.data, 0, Partlib.data_size);  //Clear it

		memcpy(&Partlib.Index, TPWM.unpacked_data + Partlib.index_offset, TPWM.unpacked_size - Partlib.index_offset); //Get the index
		memcpy(Partlib.data, TPWM.unpacked_data + 4, Partlib.data_size);  //Get the bitmap data from the uncompressed data

        free(TPWM.unpacked_data); //free buffer for unpacked data
    }
	else
	{
		// File is uncompressed

		Partlib.index_offset = ID;


		if (Partlib.index_offset > fsize)
		{
			fclose(f);
			return -3;
		}


		if ((fsize - Partlib.index_offset) != (Num_Parts * 12))
		{
			fclose(f);
			return -3;
		}


		Partlib.data_size = Partlib.index_offset - 4;

        if ((Partlib.data = (unsigned char*)malloc(Partlib.data_size)) == NULL)
		{
			fclose(f);
			return -4;  //Create Buffer
		}

		memset(Partlib.data, 0, Partlib.data_size);  //Clear it;

		IO_result = fread(Partlib.data, 1, Partlib.data_size, f); //Read bitmap data into buffer

		if (IO_result != Partlib.data_size)
		{
			fclose(f);
			return -2; //Read Error
		}

		IO_result = fread(Partlib.Index, 1, fsize - Partlib.index_offset, f); //Read the index
		if (IO_result != fsize - Partlib.index_offset)
		{
			fclose(f);
			return -2; //Read Error
		}

		fclose(f);
	}

	//====== Parts.dat

    fopen_s(&f, partdat_filename.data(), "rb");
	if (!f)
	{
		return -1;
	}

    IO_result = fread(&ID, sizeof(ID), 1, f);
	if (IO_result != 1)
	{
		fclose(f);
		return -2; //Read Error
	}

	fseek(f, 0, SEEK_END); //Get file size and set file pointer to 0 again
	fsize = ftell(f);
	rewind(f);

	if (ID == 0x4D575054) // "TPWM" String?
	{

		//File is packed
		if (Unpack_file(f) != 0)
		{
			free(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		if (TPWM.unpacked_size != (Num_Parts * 8))
		{
			free(TPWM.unpacked_data);
			return -3; //File is corrupted
		}

		memcpy(&Partdat.name, TPWM.unpacked_data, TPWM.unpacked_size); //Get Data
		free(TPWM.unpacked_data); //Free buffer for packed data - we don't need it anymore

	}
	else
	{
		//File is unpacked


		if (fsize != (Num_Parts * 8))
		{
			fclose(f);
			return -3; //File is corrupted
		}

		IO_result = fread(&Partdat.name, fsize, 1, f);
		if (IO_result != 1)
		{
			fclose(f);
			return -2; //Read Error
		}

		fclose(f);
	}

	return 0;
}





int Load_Unit_files(std::string unitlib_filename,std::string unitdat_filename)
//Load all unit data into memory (UNIT.LIB and UNIT.DAT)
{
    FILE*                       f;
    size_t                      IO_result;
    unsigned int				fsize;




	fopen_s(&f, unitlib_filename.data(), "rb");

	if (!f)
	{
		return -1;
	}

	fseek(f, 0, SEEK_END);	//Get file size
	fsize = ftell(f);
	rewind(f);


	IO_result = fread(&ID, sizeof(ID), 1, f); //Read ID

	if (IO_result != 1)
	{
		fclose(f);
		return -2; //Read Error
	}




	if (ID == 0x4D575054) //"TPWM" string?
	{
        //Yes, so file is TPWM packed

		if (Unpack_file(f) != 0)
		{
			free(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}


		memcpy(&Unitlib.index_offset, TPWM.unpacked_data, sizeof(Unitlib.index_offset)); //Get index offset

		//Check file integrity

		if (Unitlib.index_offset >= TPWM.unpacked_size)
		{
			free(TPWM.unpacked_data);
			return -3;
		}
		

		if ((TPWM.unpacked_size - Unitlib.index_offset) != sizeof(Unitlib.Index))
		{
			free(TPWM.unpacked_data);
			return -3;
		}
		
		Unitlib.data_size = Unitlib.index_offset - 4;
        if ((Unitlib.data = (unsigned char*)malloc(Unitlib.data_size)) == NULL)
		{
			free(TPWM.unpacked_data); //free buffer for unpacked data	
			return -4; //Create buffer for bitmap data
		}
		memset(Unitlib.data, 0, Unitlib.data_size);  //Clear it

		memcpy(&Unitlib.Index, TPWM.unpacked_data + Unitlib.index_offset, TPWM.unpacked_size - Unitlib.index_offset); //Get the index
		memcpy(Unitlib.data, TPWM.unpacked_data + 4, Unitlib.data_size);  //Get the bitmap data from the uncompressed data

		free(TPWM.unpacked_data); //free buffer for unpacked data	

	}
	else
	{
		// File is uncompressed

		Unitlib.index_offset = ID;


		if (Unitlib.index_offset > fsize)
		{
			fclose(f);
			return -3;
        }


		if ((fsize - Unitlib.index_offset) != sizeof(Unitlib.Index))
		{
			fclose(f);
			return -3;
		}


		Unitlib.data_size = Unitlib.index_offset - 4;

        if ((Unitlib.data = (unsigned char*)malloc(Unitlib.data_size)) == NULL)
		{
			fclose(f);
			return -4;  //Create Buffer
		}

		memset(Unitlib.data, 0, Unitlib.data_size);  //Clear it;

		IO_result = fread(Unitlib.data, 1, Unitlib.data_size, f); //Read bitmap data into buffer

		if (IO_result != Unitlib.data_size)
		{
			fclose(f);
			return -2; //Read Error
		}

		IO_result = fread(Unitlib.Index, 1, fsize - Unitlib.index_offset, f); //Read the index
		if (IO_result != fsize - Unitlib.index_offset)
		{
			fclose(f);
			return -2; //Read Error
		}      
		fclose(f);
	}

	//====== Unit.dat



	fopen_s(&f, unitdat_filename.data(), "rb");
	if (!f)
	{
		return -1;
	}

	IO_result = fread(&ID, sizeof(ID), 1, f);
	if (IO_result != 1)
	{
		fclose(f);
		return -2; //Read Error
	}


	fseek(f, 0, SEEK_END); //Get file size and set file pointer to 0 again
	fsize = ftell(f);
	rewind(f);

	
	if (ID == 0x4D575054) // "TPWM" String?
    {
        //File is packed
		if (Unpack_file(f) != 0)
		{
			free(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}
			
		if (TPWM.unpacked_size != sizeof(Unitdat.name))
		{
			free(TPWM.unpacked_data);
			return -3; //File is corrupted
		}

		memcpy(&Unitdat.name, TPWM.unpacked_data, TPWM.unpacked_size); //Get Data
		free(TPWM.unpacked_data); //Free buffer for packed data - we don't need it anymore
	}
	else
	{
		//File is unpacked

        if (fsize != ((Num_Units*6) * 8))
		{
			fclose(f);
			return -3; //File is corrupted
		}

		IO_result = fread(&Unitdat.name, fsize, 1, f);
		if (IO_result != 1)
		{
			fclose(f);
			return -2; //Read Error
		}

		fclose(f);
	}

	return 0;
}



void draw_8BPP(int pos_x, int pos_y, QImage *Image)
{
  int start_x = 0;
  int blocks = 0;
  int p = 0;
  int y = 0;
  int x = 0;
  int i = 0;
  unsigned char color = 0;


  
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
              color = Partlib.data[bitmap_offset];  //Get next unsigned char
			  ++bitmap_offset;
			  if (color != BM_Header.transparent)
                  Image->setPixel(pos_x + x, pos_y + y, qRgb(HL_Palette[color].Red,HL_Palette[color].Green,HL_Palette[color].Blue));

	      }
	  }

      }
      ++start_x;

   }
 
  
  return;
}




void draw_4BPP(int pos_x, int pos_y, int side, QImage *Image)
{
  int start_x = 0;
  int blocks = 0;
  int p = 0;
  int y = 0;
  int x = 0;
  int i = 0;
  unsigned char color = 0;
  unsigned char color1 = 0;
  unsigned char color2 = 0;
  
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
                color = Unitlib.data[bitmap_offset];  //Get next unsigned char
				++bitmap_offset;
		 
                color1 = (unsigned char) (color  >> 4);
                color2 = (unsigned char) (color & 0x0F);
                if (color1 != BM_Header.transparent)
                    Image->setPixel(pos_x + x, pos_y + y, qRgb(HL_Palette[color1+side].Red,HL_Palette[color1+side].Green,HL_Palette[color1+side].Blue));
            }
			else
                if (color2 != BM_Header.transparent)
                Image->setPixel(pos_x + x, pos_y + y, qRgb(HL_Palette[color2+side].Red,HL_Palette[color2+side].Green,HL_Palette[color2+side].Blue));

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
    int                 i, o;
    std::string         str;

	o = -1;
	for (i = 0; i <= Num_Parts; ++i)
	{
        str = char2string(Partlib.Index[i].RES_Name, 8);
		if (str.compare(char2string(Partdat.name[part_num], 8)) == 0)
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
    int                i, o;
    std::string        str;

	o = -1;
	for (i = 0; i <= (Num_Units*6); ++i)
	{
		str = char2string(Unitlib.Index[i].RES_Name, 8);
		if (str.compare(char2string(Unitdat.name[unit_num], 8)) == 0)
		{
			o = i;
			break;
		}
	}	
	
	return o;
}

int Draw_Part(int xpos, int ypos, int part_num, QImage *Image)
{
	if (part_num > Num_Parts) part_num = Num_Parts; //Invalid Part number -> draw "Verboten"
    if (Image == NULL) return -1;
	part_num = Translate_Partnum(part_num);

	bitmap_offset = Partlib.Index[part_num].Offset - 4;
	if (bitmap_offset > (Partlib.data_size - sizeof(BM_Header))) return -1;

	memcpy(&BM_Header, &Partlib.data[bitmap_offset], sizeof(BM_Header));
	bitmap_offset = bitmap_offset + sizeof(BM_Header);
	if ((bitmap_offset + (BM_Header.width * BM_Header.height)) > Partlib.data_size) return -1;

    draw_8BPP(xpos, ypos, Image);

	return 0;
}

int Draw_Unit(int xpos, int ypos, int unit_num, int side, QImage *Image)
{
	if (unit_num > Num_Units*6) return -1;
	
	unit_num = Translate_Unitnum(unit_num);
	if (unit_num == -1) return -1;

	bitmap_offset = Unitlib.Index[unit_num].Offset - 4;
	if (bitmap_offset > (Unitlib.data_size - sizeof(BM_Header))) return -1;

	memcpy(&BM_Header, &Unitlib.data[bitmap_offset], sizeof(BM_Header));
	bitmap_offset = bitmap_offset + sizeof(BM_Header);
	if ((bitmap_offset + (BM_Header.width * BM_Header.height)) > Partlib.data_size) return -1;

    draw_4BPP(xpos, ypos,side, Image);
	return 0;
}















