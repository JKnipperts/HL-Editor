/*
CODES.H by Jan Knipperts
Provides all the functions to handle the levelcode file CODES.DAT of History Line 1914-1918
Version 1.0 - 10.10.2023

The main features:

Get_Levelcodes
Reads and decodes the level codes from the game's Codes.dat (must be specified as string) and stores them in a Levelcode_info data structure.

Levelcode_exists
Checks if a level code has already been assigned. The level codes must have been read in beforehand with "Get_Levelcodes".

Add_map
Adds a new level code to the Codes.dat file


Note:
Still no possibility to change the options of the maps in the Codes.dat. Except the season, this is controlled by the global boolean variable "summer".
New maps are currently always treated as 2-player maps.


*/


#include <iostream>
#include <string>
#include <sstream>
#include <QImage>


struct Levelcode_info
{
    int                     Number_of_levels;
    QStringList             Codelist;
};

struct Mapoption_info
{
    unsigned char           unknown1 = 2;
    unsigned char           map_type = 2;  //set to 2-player map
    unsigned char           unknown2 = 1;
    unsigned char           season = 2;
    unsigned char           unknown3 = 0;
};



Levelcode_info              Levelcode;
Mapoption_info              Mapoptions;



int Get_Levelcodes(std::string codes_filename)
{
    FILE*                   f;
    size_t                  IO_result;
    int                     i,o;
    unsigned int            fsize;
    unsigned int			ID;
    unsigned char*          ASCII_buffer;

    fopen_s(&f, codes_filename.data(), "rb");

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

        ASCII_buffer = TPWM.unpacked_data;
        fsize = TPWM.unpacked_size;
    }
    else  //File is not packed
    {
        ASCII_buffer = (unsigned char*)malloc(fsize);
        if (ASCII_buffer == NULL) //Reserve memory buffer
        {
            fclose(f);
            return -4;
        }

        memset(ASCII_buffer, 0, fsize);  //Clear memory;

        rewind(f);
        IO_result = fread(ASCII_buffer, fsize, 1, f); //Read whole file into memory

        if (IO_result != 1) //Read Error?
        {
            fclose(f);
            free(ASCII_buffer);
            return -2;
        }

        fclose(f);
    }



    QString code;
    int offset;
    int ASCII;
    Levelcode.Codelist.clear();
    Levelcode.Number_of_levels = (fsize/10);


    offset = 0;
    for (i = 0;i < Levelcode.Number_of_levels; i++)
    {
       code = "";
       for (o = 0; o < 5; o++)
       {
            ASCII = ASCII_buffer[offset+o];
            code[o] = QChar::fromLatin1(ASCII+48); //17 = "A" in CODES.DAT, 65 (ASCII "A")-17 = 48;
       }
       offset = offset+10; //Skip data, we just want the passwords
       Levelcode.Codelist.append(code);
    }
    free(ASCII_buffer);



    return 0;
}


bool Levelcode_exists(QString testcode)
{
    int i;
    for (i=0;i < Levelcode.Codelist.count(); i++)
    {
        if (QString::compare(Levelcode.Codelist[i], testcode, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}



int Add_map(std::string codes_filename, QString newcode)
{
    FILE*                   f;
    size_t                  IO_result;
    unsigned int			ID;

    fopen_s(&f, codes_filename.data(), "rb+");

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


    if (ID == 0x4D575054) //"TPWM" so file is compressed...
    {
        if (Unpack_file(f) != 0)
        {
            free(TPWM.unpacked_data);
            return -5;		//Decompressing failed!
        }



        fopen_s(&f, codes_filename.data(), "wb");
        //Overwrite file with unpacked data

        IO_result = fwrite(TPWM.unpacked_data,TPWM.unpacked_size,1, f);
        if (IO_result != 1)
        {
            fclose(f);
            return -2; //Write Error
        }

    }

    fseek(f, 0, SEEK_END);	//seek to end of file

    newcode = newcode.toUpper();

    //Crypt code and write it to file
    for (int i = 0; i < 5; i++)
    {
        char ascii = newcode.at(i).toLatin1(); //17 = "A" in CODES.DAT, 65 (ASCII "A")
        ascii = ascii - 0x30;
        IO_result = fwrite(&ascii,1,1, f);
        if (IO_result != 1)
        {
            fclose(f);
            return -2; //Write Error
        }
    }

    if (summer)
        Mapoptions.season = 0;
    else
        Mapoptions.season = 1;

    IO_result = fwrite(&Mapoptions,sizeof(Mapoptions),1, f);
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }
    fclose(f);


    return 0;
}

