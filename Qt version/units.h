/*
UNITS.H by Jan Knipperts
Reads unit names from UNITS.DAT of History Line 1914-1918
Version 1.0 - 27.11.2023

Info:
The UNIT.DAT file in HL's main directory contains information of all units (name, group size, attack value etc).
It seems that there is a 69 Byte chunk for each unit. The unit's name is at offset 0x2C of each chunk.
*/

QString Unit_Name[Num_Units];

int Get_unit_names(std::string unitfilename)
{
    FILE*                   f;
    size_t                  IO_result;
    unsigned int            fsize;
    unsigned int			ID;
    unsigned int            i;
    unsigned int            offset;
    unsigned char           c;



    fopen_s(&f, unitfilename.data(), "rb");

    if (!f) return -1; //File not found


    IO_result = fread(&ID, sizeof(ID), 1, f); //Read ID

    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Read Error
    }

    fseek(f, 0, SEEK_END);	//Get file size
    fsize = ftell(f);

    if (ID == 0x4D575054) //"TPWM" string - file is compressed
    {
        if (Unpack_file(f) != 0)
        {
            free(TPWM.unpacked_data);
            return -5;		//Decompressing failed!
        }
        fsize = TPWM.unpacked_size;

        if (fsize != (Num_Units*0x45)) //File is corrupt return -6;

        for (i = 0; i < Num_Units; i++)
        {
            offset = (i*0x45)+0x2C;
            c = 1;
            Unit_Name[i] = "";
            while ((c > 0) && (offset <= (i*0x45)+0x45)) //Unit names are stored as 0 terminated strings in the file
            {
                c = (unsigned char) TPWM.unpacked_data[offset];

                switch (c)      //Handle german Umlaute...
                {
                case 0x8E:
                    Unit_Name[i] = Unit_Name[i] + "AE";
                    break;

                case 0x99:
                    Unit_Name[i] = Unit_Name[i] + "OE";
                    break;

                default:
                    Unit_Name[i] = Unit_Name[i] + QChar::fromLatin1(c);
                    break;
                }

                offset++;
            }
        }

    }
    else
    {
        if (fsize != (Num_Units*0x45)) //File is corrupt
        {
            fclose(f);
            return -6;
        }

        for (i = 0; i < Num_Units; i++)
        {
            fseek(f,(i*0x45)+0x2C, SEEK_SET);
            c = 1;
            Unit_Name[i] = "";
            while (c > 0)  //Unit names are stored as 0 terminated strings in the file
            {
                IO_result = fread(&c,1, 1, f);
                if (IO_result != 1) //Read Error?
                {
                    fclose(f);
                    return -2;
                }

                switch (c)
                {
                case 0x8E:
                    Unit_Name[i] = Unit_Name[i] + "AE";
                    break;

                case 0x99:
                    Unit_Name[i] = Unit_Name[i] + "OE";
                    break;

                default:
                    Unit_Name[i] = Unit_Name[i] + QChar::fromLatin1(c);
                    break;
                }

            }
        }
        fclose(f);
    }
    return 0;
}










