/* FIN.H by Jan Knipperts
Header file to read and write History Line 1914-1918 map files (*.fin)

Version 1.3 - 20.12.2023
- used_tiles added  to be able to track how many terrain parts have been used so far.

Load_Mapdata
Loads a map from file

Save_Mapdata
Saves data from "map" record to a file

Draw_Map
Draws the actual map to a screen buffer. Requires lib.h

*/


struct map_info
{
    unsigned short int		width;
    unsigned short int		height;
    unsigned char*          data;
    unsigned int            data_size;
    bool                    loaded = false;
};

struct field_info
{
    unsigned char           Part;
    unsigned char           Unit;
};


map_info                    Map;
field_info                  Field;

const int                   Tilesize = 24;		//Ein Hexfeld hat einen Durchmesser von 24 Pixeln
const int                   Tileshift = 8;		//Versatz der Hexfelder


int Load_Mapdata(std::string mapname)
{
    FILE*               f;
    size_t              IO_result;
    unsigned int		ID;
    unsigned int		fsize;

    if (Map.data != NULL) free(Map.data);

    fopen_s(&f, mapname.data(), "rb");

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
	rewind(f);


    if (ID == 0x4D575054) //"TPWM" ID string?
	{

		if (Unpack_file(f) != 0)
		{
			free(TPWM.unpacked_data);
			return -5;		//Decompressing failed!
        }

        memcpy(&Map.width, TPWM.unpacked_data, 2);  //Get width and height of map
        memcpy(&Map.height, TPWM.unpacked_data+2, 2);

        Map.width = (Map.width >> 8) | (Map.width << 8);  //Big endian conversion
        Map.height = (Map.height >> 8) | (Map.height << 8);
		
        Map.data_size = (Map.width * Map.height) * 2;

		if (Map.data_size != TPWM.unpacked_size - 4)
		{				
			free(TPWM.unpacked_data);
			return -3; //File corrupt
		}

        if ((Map.data = (unsigned char*)malloc(Map.data_size)) == NULL)
        {
            free(TPWM.unpacked_data);
            return -4;
        }
        memset(Map.data, 0, Map.data_size);  //Clear it
		memcpy(Map.data, TPWM.unpacked_data+4, Map.data_size);
		free(TPWM.unpacked_data);				

	}
	else
	{
        IO_result = fread(&Map.width, sizeof(Map.width), 1, f); //Read width

        if (IO_result != 1)
        {
            fclose(f);
            return -2; //Read Error
        }
        IO_result = fread(&Map.height, sizeof(Map.height), 1, f); //Read height

        if (IO_result != 1)
        {
            fclose(f);
            return -2; //Read Error
        }


        Map.width = (Map.width >> 8) | (Map.width << 8);  //Big endian conversion
        Map.height = (Map.height >> 8) | (Map.height << 8);

        Map.data_size = (Map.width * Map.height) * 2;


		if (Map.data_size != fsize - 4)
		{
			fclose(f);
			return -3; //File corrupt
		}

        if ((Map.data = (unsigned char*)malloc(Map.data_size)) == NULL)
		{
			fclose(f);
			return -4;
		}

		memset(Map.data, 0, Map.data_size);  //Clear it

		
		IO_result = fread(Map.data, Map.data_size, 1, f); //Read map data
		if (IO_result != 1)
		{
			fclose(f);			
			return -2; //Read Error
		}
		fclose(f);
	}

    Map.loaded = true;
	return 0;

}


int Save_Mapdata(std::string mapname)
{
    FILE*               f;
    size_t              IO_result;


    if (Map.data == NULL) return -3; //memory error, no map data in memory

    unsigned short int mwidth = (Map.width << 8) | (Map.width >> 8);   //Big / little endian conversion
    unsigned short int mheight = (Map.height << 8) | (Map.height >> 8);

    fopen_s(&f, mapname.data(), "wb");
    if (!f) return -1;

    IO_result = fwrite(&mwidth, sizeof(mwidth), 1, f); //Write map width
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }

    IO_result = fwrite(&mheight, sizeof(mheight), 1, f); //Write map height
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }

    IO_result = fwrite(Map.data, (Map.width*Map.height)*2, 1, f); //Write map data
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }

    fclose(f);
    return 0;

}




int  Draw_Map()
{
    unsigned int		offset;
	int			x,y, side,unit;
	

	offset = 0;
    side = 1;

    if ((Map.data == NULL) || (Map.loaded == false)) return -1; // No map loaded


    for (y = 0; y < Map.height; y++)
    {
        for (x = 0; x < Map.width; x++)
        {
            memcpy(&Field, Map.data + offset, sizeof(Field));
			offset = offset + sizeof(Field);
			if (Field.Part != 0xAE)
            {
                if (x % 2 != 0)
                {
                    Draw_Part((x * (Tilesize - Tileshift)), (y * Tilesize) + (Tilesize / 2), Field.Part, &MapImage);


                    if (Field.Unit != 0xFF)
                    {
                        if (Field.Unit % 2 == 0) side = 1; else side = 2;
                        unit = (Field.Unit / 2) * 6;
                        if (side == 1) unit = unit + 3;
                        Draw_Unit((x * (Tilesize - Tileshift)), (y * Tilesize) + (Tilesize / 2), unit, side, &MapImage);
                    }
                }
                else
                {
                    Draw_Part((x * (Tilesize - Tileshift)), (y * Tilesize), Field.Part, &MapImage);

                    if (Field.Unit != 0xFF)
                    {
                        if (Field.Unit % 2 == 0) side = 1; else side = 2;
                        unit = (Field.Unit / 2) * 6;
                        if (side == 1) unit = unit + 3;
                        Draw_Unit((x * (Tilesize - Tileshift)), (y * Tilesize), unit, side, &MapImage);
                    }
                }
			}
		}
	}
							

	return 0;
}
