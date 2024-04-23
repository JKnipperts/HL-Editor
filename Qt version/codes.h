/*
CODES.H by Jan Knipperts
Provides all the functions to handle the levelcode file CODES.DAT of History Line 1914-1918
Version 1.1 - 22.11.2023
- fixed "Using QCharRef with an index pointing outside the valid range of a QString."-Warning caused by Get_Levelcodes
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
    unsigned char           unknown1 = 2;  //allways 2 for all maps
    unsigned char           map_type = 2;  //1 for single player, 2 for two player map
    unsigned char           end_of_campaign = 0; //1 = jump to next map after this one is finished, 0 = return to main menu or play extro (if single player)
    unsigned char           season = 2;    //0 = map plays in summer, 1 or 2 = winter (I wonder why they used 2 values here)
    unsigned char           unknown3 = 0; //allways 0 for all maps
};



Levelcode_info              Levelcode;
Mapoption_info              Mapoptions;
unsigned char*              CODESDAT_buffer;
unsigned int                CODESDAT_size;


int Read_Codesdat(std::string codes_filename)
{
    FILE*                   f;
    size_t                  IO_result;
    unsigned int			ID;


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
    CODESDAT_size = ftell(f);

    if (ID == 0x4D575054) //"TPWM" string?
    {
        if (Unpack_file(f) != 0)
        {
            free(TPWM.unpacked_data);
            return -5;		//Decompressing failed!
        }

        CODESDAT_buffer = TPWM.unpacked_data;
        CODESDAT_size = TPWM.unpacked_size;
    }
    else  //File is not packed
    {
        CODESDAT_buffer = (unsigned char*)malloc(CODESDAT_size);
        if (CODESDAT_buffer == NULL) //Reserve memory buffer
        {
            fclose(f);
            return -4;
        }

        memset(CODESDAT_buffer, 0, CODESDAT_size);  //Clear memory;

        rewind(f);
        IO_result = fread(CODESDAT_buffer, CODESDAT_size, 1, f); //Read whole file into memory

        if (IO_result != 1) //Read Error?
        {
            fclose(f);
            free(CODESDAT_buffer);
            return -2;
        }

        fclose(f);
    }
    return 0;
}




int Get_Levelcodes()
{
    QString code;
    int offset;
    int ASCII;
    int i,o;

    if (CODESDAT_buffer != NULL)
    {
        Levelcode.Number_of_levels = (CODESDAT_size/10);
        Levelcode.Codelist.clear();
        offset = 0;
        for (i = 0;i < Levelcode.Number_of_levels; i++)
        {
            code = "";
            for (o = 0; o < 5; o++)
            {
                ASCII = CODESDAT_buffer[offset+o];
                code = code + QChar::fromLatin1(ASCII+48); //17 (0x11) = "A" in CODES.DAT, 65 (ASCII "A")-17 = 48;
            }
            offset = offset+10; //Skip data, we just want the passwords here
            Levelcode.Codelist.append(code);
        }
        return 0;
    }
    else
        return -1;
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
    unsigned int			ID, size;

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


    //Unpack the file if necessary

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

    //Correct end of campaign mark for the former last map

    fseek(f, 0, SEEK_END);	//seek to end of file
    size = ftell(f);  //Get file size
    fseek(f,size-3,SEEK_SET);
    unsigned char end_of_campaign = 1;
    IO_result = fwrite(&end_of_campaign,1,1, f);
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }
    fseek(f, 0, SEEK_END);	//seek to end of file


    //Add the new map
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

    if (Player2)
        Mapoptions.map_type = 2;
    else
        Mapoptions.map_type = 1;

    IO_result = fwrite(&Mapoptions,sizeof(Mapoptions),1, f);
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }            
    fclose(f);

    //Re-read CODES.DAT into memory

    if (Read_Codesdat(codes_filename) != 0)
    {
        QMessageBox              Errormsg;
        Errormsg.critical(0,"Error","Error reloading CODES.DAT!");
        Errormsg.setFixedSize(500,200);
        return -2;
    }
    Get_Levelcodes();  //Update our list of levelcodes

    return 0;
}


int Remove_map(std::string codes_filename, QString removecode)
{
    FILE*                   f;
    size_t                  IO_result;
    int                     i,sp;
    bool                    brokencampaign = false;

    if (CODESDAT_buffer != NULL)
    {
        for (i = 0; i < Levelcode.Codelist.count(); i++)
        {
            if (QString::compare(Levelcode.Codelist[i], removecode, Qt::CaseInsensitive) == 0) break;
        }

        if ((i >= Levelcode.Codelist.count()) || (unsigned (i*10) > CODESDAT_size)) //The given code does not exist in Codes.dat
        {
            return -1;
        }

       int remove_pos = i*10;

        if ((unsigned) remove_pos < (CODESDAT_size-10))
        {
            for (i = remove_pos; (unsigned) i < CODESDAT_size-10; i++)
            CODESDAT_buffer[i] = CODESDAT_buffer[i+10];
        }

        CODESDAT_size = CODESDAT_size-10;


        for (i = 0; (unsigned) i < (CODESDAT_size/10); i++)
        {
            if (CODESDAT_buffer[(i*10)+8] == 1) CODESDAT_buffer[(i*10)+8] = 0;
            if (CODESDAT_buffer[(i*10)+7] == 0) CODESDAT_buffer[(i*10)+7] = 1;

            if (i <= 47)
            {
              if (CODESDAT_buffer[(i*10)+6] != 1)
              {
                brokencampaign = true;
                sp = i-1;
              }
            }
        }

        if (brokencampaign)
        {
            QMessageBox              Errormsg;
            Errormsg.warning(0,"Warning:","A map has been removed from the single-player campaign. This will lead to problems when the campaign is played, as video sequences etc. can no longer be assigned correctly.");
            Errormsg.setFixedSize(500,200);

            CODESDAT_buffer[(sp*10)+8] = 1;  //New end of french campaign;
            CODESDAT_buffer[((sp/2)*10)+8] = 1; //New end of german campaign;
            CODESDAT_buffer[(sp*10)+7] = 0; //New end of single player campaign
        }
        else
        {
            CODESDAT_buffer[238] = 1;  //End of german campaign
            CODESDAT_buffer[468] = 1;  //End of french campaign
            CODESDAT_buffer[467] = 0;  //End of single player campaign
        }


         CODESDAT_buffer[CODESDAT_size-3] = 0; //Set end of campaign marker for two player and user maps;


        fopen_s(&f, codes_filename.data(), "wb");

        if (!f)
        {
            return -1;
        }
        IO_result = fwrite(CODESDAT_buffer,CODESDAT_size,1, f);
        if (IO_result != 1)
        {
             fclose(f);
            return -2; //Write Error
        }

        fclose(f);

        //Re-read CODES.DAT into memory

        if (Read_Codesdat(codes_filename) != 0) return -3;
        Get_Levelcodes();  //Update our list of levelcodes
    }
    else
        return -4;



    return 0;

}






int Get_Mapoptions(int levelnum)  //Returns the additional information for a level from the code.dat. These are returned in "Mapoptions".
{
    if ((CODESDAT_buffer != NULL) && ((unsigned int) ((levelnum*10)+5) < CODESDAT_size))
    {
      memcpy(&Mapoptions,CODESDAT_buffer+((levelnum*10)+5),sizeof(Mapoption_info));
      return 0;
    }
    else
      return -1;
}
