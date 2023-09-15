/* FIN.H - DOS Version 1.0

 Load a Mapfile into memory buffers
  -  int Load_Mapdata(char* map_filename)

 int Draw_Map_Section(int hx, int hy)
 - Draws a section of the map to "backbuffer".
   hx,hy specify the upper left hex field of the section.
   VP_WIDTH and VP_HEIGHT provide the size of the section / viewport. */








struct map_info
{
	word		x;		//Todo: Proper big endian reading
	word		y;
	byte huge*	data;
	dword		data_size;
};

struct field_info
{
	byte		Part;
	byte		Unit;
};


map_info		Map;
field_info		Field;

const int		Tilesize = 24;		//Ein Hexfeld hat einen Durchmesser von 24 Pixeln
const int		Tileshift = 8;		//Versatz der Hexfelder




int Load_Mapdata(char* map_filename)
{
	FILE*		mf;
	size_t		IO_result;
	dword		ID;
	dword		fsize;
	byte*		IO_buffer;

	if (Map.data != NULL) farfree(Map.data);

	mf = fopen(map_filename, "rb");

	if (!mf)
	{
		return -1;
	}

	IO_result = fread(&ID, sizeof(ID), 1, mf); //Read ID

	if (IO_result != 1)
	{
		fclose(mf);
		return -2; //Read Error
	}

	fseek(mf, 0, SEEK_END);	//Get file size
	fsize = ftell(mf);
	rewind(mf);



	if (ID == 0x4D575054) //"TPWM" string?
	{
		if (Unpack_file(mf) != 0)
		{
			farfree(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
		}

		Map.x =  TPWM.unpacked_data[0] | (TPWM.unpacked_data[1] << 8);
		Map.y =  TPWM.unpacked_data[2] | (TPWM.unpacked_data[3] << 8);

		Map.x = (Map.x >> 8) | (Map.x << 8);  //Big endian conversion
		Map.y = (Map.y >> 8) | (Map.y << 8);

		Map.data_size = (Map.x * Map.y) * 2;

		if (Map.data_size != TPWM.unpacked_size - 4)
		{
			farfree(TPWM.unpacked_data);
			return -3; //File corrupt
		}

		if ((Map.data = (byte*)malloc(Map.data_size)) == NULL)
		{
			farfree(TPWM.unpacked_data);
			return -4;
		}
		_fmemset(Map.data, 0, Map.data_size);  //Clear it
		_fmemcpy(Map.data, TPWM.unpacked_data+4, Map.data_size);
		farfree(TPWM.unpacked_data);

	}
	else
	{
		IO_result = fread(&Map.x, sizeof(Map.x), 1, mf); //Read X

		if (IO_result != 1)
		{
			fclose(mf);
			return -2; //Read Error
		}
		IO_result = fread(&Map.y, sizeof(Map.y), 1, mf); //Read Y

		if (IO_result != 1)
		{
			fclose(mf);
			return -2; //Read Error
		}


		Map.x = (Map.x >> 8) | (Map.x << 8);  //Big endian conversion
		Map.y = (Map.y >> 8) | (Map.y << 8);
		Map.data_size = (Map.x * Map.y) * 2;

		if (Map.data_size != fsize - 4)
		{
			fclose(mf);
			return -3; //File corrupt
		}

		if ((Map.data = (byte*)malloc(Map.data_size)) == NULL)
		{
			fclose(mf);
			return -4;
		}


		IO_buffer = (unsigned char*) malloc(Map.data_size);
		memset(IO_buffer,0,Map.data_size);

		if (IO_buffer == NULL)
		{
			fclose(mf);
			return -4; //Reserve memory buffer failed
		}

		IO_result = fread(IO_buffer, Map.data_size, 1, mf); //Read map data
		if (IO_result != 1)
		{
			fclose(mf);
			return -2; //Read Error
		}
		fclose(mf);

		_fmemset(Map.data, 0, Map.data_size);  //Clear it
		_fmemcpy(Map.data,MK_FP(FP_SEG(IO_buffer),FP_OFF(IO_buffer)),Map.data_size);
		free(IO_buffer);

	}

	return 0;

}


int Draw_Map_Section(int hx, int hy)
{
 int x;
 int y;
 int offset;
 int side;
 int unit;

	if (Map.data == NULL) return -1; // No map loaded

	if ((hx > Map.x) |
	    (hy > Map.y))
	return -1;			//Section not valid

	for (y = 0; y < VP_HEIGHT; y++)
	{
	 if (hy+y > Map.y) break;

	for (x = 0; x < VP_WIDTH; x++)
	{
	  if (hx+x > Map.x) break;
	  offset = (((hy+y)*Map.x)+(hx+x))*2;
	  if (offset > Map.data_size) break;

	  Field.Part = (byte) Map.data[offset];
	  Field.Unit = (byte) Map.data[offset+1];

	  if (Field.Part == 0xAE) break;


	  if (Field.Part != 0xAE)
	  {
	    if ((hx+x) % 2 != 0)
	    {
		Draw_Part(Backbuffer, (x * (Tilesize - Tileshift))+3, (y * Tilesize) + (Tilesize / 2), Field.Part);

		if (Field.Unit != 0xFF)
		{
		  if (Field.Unit % 2 == 0) side = 1; else side = 2;
		  unit = (Field.Unit / 2) * 6;
		  if (side == 1) unit = unit + 3;
		  Draw_Unit(Backbuffer, (x * (Tilesize - Tileshift))+3, (y * Tilesize) + (Tilesize / 2), unit, side);
		}
	    }
	    else
	    {
		Draw_Part(Backbuffer, (x * (Tilesize - Tileshift))+3, (y * Tilesize), Field.Part);
		if (Field.Unit != 0xFF)
		{
		  if (Field.Unit % 2 == 0) side = 1; else side = 2;
		  unit = (Field.Unit / 2) * 6;
		  if (side == 1) unit = unit + 3;
		 Draw_Unit(Backbuffer, (x * (Tilesize - Tileshift))+3, (y * Tilesize), unit, side);
		}
	     }
	  }
	}
	}
	return 0;
}
