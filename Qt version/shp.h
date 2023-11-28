/* SHP.H by Jan Knipperts
Header file to read and write History Line 1914-1918 map files (*.shp)

Version 1.2 - 27.11.2023
- Fixed handling of not saved transport units in the original game files

*/




struct Building_data //Date structure in shp file
{
    unsigned char  Owner;     // 	To which side the building belongs (0 = German; 1 = French, 2 = Neutral)
    unsigned char  Type;      //	Type of building (0=HQ; 1 = Factory; 2 = Depot; 3 = Transport unit)
    unsigned char  Index;     //	xth unit/building of this type
    unsigned char  Resources; //    number of resources this building generates per turn
    unsigned char  Unknown;   //    The meaning of this byte still has to be found out.
    unsigned char  Units[7];  //	Units inside the building/transport unit
};

struct  Building_data_ext  //Extanded structure for our use
{
    unsigned int          Field;
    Building_data*        Properties;
};

struct  SHP_struct
{
    unsigned char   can_be_built[Num_Units];
    unsigned char   num_buildings;
    Building_data*  buildings;
};

struct Building_statistics
{
    unsigned char   num_HQ; //number of HQ    
    unsigned char   num_F;  //number of fabrications  
    unsigned char   num_D; //number of depots    
    unsigned char   num_T; //number of transport units    
    unsigned char   num_buildings; //total number of buildings    
};


SHP_struct          SHP;
Building_data_ext*  Building_info;
Building_statistics Building_stat;
Building_data       Bdata;


int Read_shp_data(std::string shpname)
{
    FILE*                   f;
    size_t                  IO_result;
    unsigned int            fsize;
    unsigned int			ID;
    unsigned int            i;


    fopen_s(&f, shpname.data(), "rb");

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

        if (fsize < sizeof(SHP.can_be_built)+1) return -6; //File is corrupt

        memcpy(&SHP.can_be_built, TPWM.unpacked_data, sizeof(SHP.can_be_built));

        memcpy(&SHP.num_buildings, TPWM.unpacked_data+sizeof(SHP.can_be_built), sizeof(SHP.num_buildings));

        if (fsize < (sizeof(SHP.can_be_built)+(SHP.num_buildings*sizeof(Building_data))+1)) return -6; //File is corrupt

        if (SHP.num_buildings > 0)
        {
            SHP.buildings = (Building_data*) malloc(SHP.num_buildings*sizeof(Building_data));
            if (SHP.buildings == NULL)  return -4; //Not enough mem?
            memset(SHP.buildings, 0, SHP.num_buildings*sizeof(Building_data));  //Clear
            memcpy(SHP.buildings, TPWM.unpacked_data+sizeof(SHP.can_be_built)+1, SHP.num_buildings*sizeof(Building_data));
        }
    }
    else
    {
        rewind(f);


        if (fsize < sizeof(SHP.can_be_built)+1)
        {
            fclose(f);
            return -6; //File is corrupt
        }

        IO_result = fread(&SHP.can_be_built, sizeof(SHP.can_be_built), 1, f);

        if (IO_result != 1) //Read Error?
        {
            fclose(f);
            return -2;
        }

        IO_result = fread(&SHP.num_buildings, sizeof(SHP.num_buildings), 1, f);

        if (IO_result != 1) //Read Error?
        {
            fclose(f);
            return -2;
        }


        if (SHP.num_buildings > 0)
        {
            if (fsize != (ftell(f)+(SHP.num_buildings*sizeof(Building_data))))
            {
                fclose(f);
                return -6; //File is corrupt
            }


            SHP.buildings = (Building_data*) malloc(SHP.num_buildings*sizeof(Building_data));
            if (SHP.buildings == NULL) //Not enough mem?
            {
                fclose(f);
                return -4;
            }

            IO_result = fread(SHP.buildings,(SHP.num_buildings*sizeof(Building_data)), 1, f);

            if (IO_result != 1) //Read Error?
            {
                fclose(f);
                return -2;
            }
        }
        fclose(f);

    }


    Building_info = (Building_data_ext*) malloc(SHP.num_buildings*sizeof(Building_data_ext));
    if (Building_info == NULL) return -4; //Not enough mem?
    memset(Building_info, 0, SHP.num_buildings*sizeof(Building_data_ext));  //Clear

    for (i = 0; i < SHP.num_buildings; i++)
        Building_info[i].Properties = &SHP.buildings[i]; //Let our extanded infos point to infos from shp file


    return 0;
}


void Add_building_positions()
{
    int i,offset,hqc,fc,dc,tc,otc, owner;

    offset = 0;
    hqc = 0; //Counter for HQs
    fc = 0; //Counter for Factories
    dc = 0; //Counter for Depots
    tc = 0; //Counter for transport units

    Building_stat.num_buildings = SHP.num_buildings;


    if ((Map.loaded) && (Building_info != NULL))
    {

      for (offset = 0; offset < ((Map.width*Map.height)*2); offset+=2)
      {
            memcpy(&Field, Map.data + offset, sizeof(Field));

            if ( (Field.Part == 0x01) ||
                (Field.Part == 0x02)) // HQ
            {
              owner = Field.Part-1;

                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 0) &&
                        (Building_info[i].Properties->Owner == owner))
                    {
                        Building_info[i].Field = offset/2;
                        hqc++;                
                        break;
                    }
                }                               

            }


            if ( (Field.Part == 0x0C) || // Factory neutral
                (Field.Part == 0x0D) ||  // Factory German
                (Field.Part == 0x0E))   // Factory French
            {
                switch (Field.Part)
                {
                case 0x0C:
                    owner = 2;
                    break;
                case 0x0D:
                    owner = 0;
                    break;
                case 0x0E:
                    owner = 1;
                    break;
                }

                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 1) &&
                        (Building_info[i].Properties->Owner == owner) &&
                        (Building_info[i].Properties->Index == fc))
                    {
                        Building_info[i].Field = offset/2;
                        fc++;                
                        break;
                    }

                }
            }

            if ( (Field.Part == 0x0F) ||
                (Field.Part == 0x10) ||
                (Field.Part == 0x11)) // Depot
            {
                switch (Field.Part)
                {
                case 0x0F:
                    owner = 2;
                    break;
                case 0x10:
                    owner = 0;
                    break;
                case 0x11:
                    owner = 1;
                    break;
                }



                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 2) &&
                        (Building_info[i].Properties->Owner == owner) &&
                        (Building_info[i].Properties->Index == dc))
                    {
                        Building_info[i].Field = offset/2;
                        dc++;                
                        break;
                    }

                }
            }

            if  ((Field.Unit == 0x2C) ||
                 (Field.Unit == 0x2D))
            {
                owner = Field.Unit-0x2C;
                otc = tc;

                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 3) &&
                        (Building_info[i].Properties->Owner == owner) &&
                        (Building_info[i].Properties->Index == tc))
                    {
                        Building_info[i].Field = offset/2;
                        tc++;
                        break;
                    }

                }

                // Bluebyte has apparently only saved transport units with content in the SHP files.
                // For a field with a transport unit without an associated data record, we simply create an empty one.

                if (tc == otc)
                {
                    Building_stat.num_buildings++;
                    if (Building_info != NULL)
                        Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));
                    else
                        Building_info = (Building_data_ext*) malloc(Building_stat.num_buildings*sizeof(Building_data_ext));

                    Building_info[Building_stat.num_buildings-1].Field = offset/2;
                    Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 3;
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = owner;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = tc;
                    Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;

                    tc++;
                }


            }


            if  ((Field.Unit == 0x34) ||
                 (Field.Unit == 0x35))
            {
                owner = Field.Unit-0x34;
                otc = tc;

                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 3) &&
                        (Building_info[i].Properties->Owner == owner) &&
                        (Building_info[i].Properties->Index == tc))
                    {
                        Building_info[i].Field = offset/2;
                        tc++;
                        break;
                    }                                        
                }                

                // Bluebyte has apparently only saved transport units with content in the SHP files.
                // For a field with a transport unit without an associated data record, we simply create an empty one.

                if (tc == otc)
                {
                    Building_stat.num_buildings++;
                    if (Building_info != NULL)
                        Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));
                    else
                        Building_info = (Building_data_ext*) malloc(Building_stat.num_buildings*sizeof(Building_data_ext));

                    Building_info[Building_stat.num_buildings-1].Field = offset/2;
                    Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 3;
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = owner;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = tc;
                    Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;

                    tc++;
                }

            }

            if  ((Field.Unit == 0x3E) ||
                 (Field.Unit == 0x3F))
            {
                owner = Field.Unit-0x3E;
                otc = tc;

                for (i = 0; i < SHP.num_buildings; i++)
                {
                    if ((Building_info[i].Properties->Type == 3) &&
                        (Building_info[i].Properties->Owner == owner) &&
                        (Building_info[i].Properties->Index == tc))
                    {
                        Building_info[i].Field = offset/2;
                        tc++;
                        break;
                    }
                }


                // Bluebyte has apparently only saved transport units with content in the SHP files.
                // For a field with a transport unit without an associated data record, we simply create an empty one.

                if (tc == otc)
                {
                    Building_stat.num_buildings++;
                    if (Building_info != NULL)
                        Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));
                    else
                        Building_info = (Building_data_ext*) malloc(Building_stat.num_buildings*sizeof(Building_data_ext));

                    Building_info[Building_stat.num_buildings-1].Field = offset/2;
                    Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 3;
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = owner;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = tc;
                    Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;

                    tc++;
                }
            }
        }
      }


      Building_stat.num_HQ = hqc;
      Building_stat.num_F = fc;
      Building_stat.num_D = dc;
      Building_stat.num_T = tc;
      Building_stat.num_buildings = (hqc+fc+dc+tc); //correct statistics

}


int Get_Buildings_info(int fieldno)
{
    int i;

    for (i = 0; i < Building_stat.num_buildings; i++)
    {
    if (Building_info[i].Field == fieldno)
        return i;
    }

    return -1; //not found
}



void Create_building_record_from_map()
{
    int offset;
    memset(&Building_stat,0,sizeof(Building_stat));


    if (Building_info != NULL) free(Building_info);
    if (SHP.buildings != NULL) free(SHP.buildings);

    Building_info = (Building_data_ext*) malloc(sizeof(Building_data_ext));


    if (Map.loaded)
    {

    for (offset = 0; offset < ((Map.width*Map.height)*2); offset+=2)
    {
        memcpy(&Field, Map.data + offset, sizeof(Field));

        if (((Field.Part == 0x01) || (Field.Part == 0x02)) ||
            ((Field.Part >= 0x0C) && (Field.Part <= 0x11)) ||
            ((Field.Unit == 0x2C) ||
             (Field.Unit  == 0x2D) ||
             (Field.Unit  == 0x34) ||
             (Field.Unit  == 0x35) ||
             (Field.Unit  == 0x3E) ||
             (Field.Unit  == 0x3F)))
            {
                Building_stat.num_buildings++;

                if (Building_info != NULL)
                    Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));


                Building_info[Building_stat.num_buildings-1].Field = offset/2;
                Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));


                switch (Field.Part)
                {
                case 0x01:
                {                
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_HQ;
                    Building_stat.num_HQ++;                
                    break;
                }
                case 0x02:
                {
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_HQ;
                    Building_stat.num_HQ++;                 
                    break;
                }
                case 0x0C:  //Factory neutral
                {                    
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 2;  //neutral
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                    Building_stat.num_F++;              
                    break;
                }
                case 0x0D:  //Factory german
                {                    
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                    Building_stat.num_F++;                
                    break;
                }
                case 0x0E:  //Factory french
                {                    
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                    Building_stat.num_F++;               
                    break;
                }
                case 0x0F:  //Depot neutral
                {                    
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 2;  //neutral
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                    Building_stat.num_D++;                
                    break;
                }
                case 0x10:  //Depot german
                {                    
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                    Building_stat.num_D++;                  
                    break;
                }
                case 0x11:  //Depot french
                {
                    Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french
                    Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                    Building_stat.num_D++;               
                    break;
                }
                }



                if ((Field.Unit == 0x2C) ||
                    (Field.Unit == 0x2D) ||
                    (Field.Unit == 0x34) ||
                    (Field.Unit == 0x35) ||
                    (Field.Unit == 0x3E) ||
                    (Field.Unit == 0x3F)) //Transport)
                {
                    if ((Field.Unit == 0x2C) || (Field.Unit == 0x34) || (Field.Unit == 0x3E))
                    {
                        Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german                 
                    }
                    else
                    {
                        Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french                       
                    }

                    Building_info[Building_stat.num_buildings-1].Properties->Type = 3;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_T;
                    Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                    Building_stat.num_T++;
                }

                Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;
           }
        }
    }

}



int Create_shp(std::string shpname)
{
    FILE*                   f;
    size_t                  IO_result;
    int                     fi,di,ti,i;
    int                     index,offset;

    fopen_s(&f, shpname.data(), "wb");
    if (!f) return -1;

    IO_result = fwrite(&SHP.can_be_built, sizeof(SHP.can_be_built), 1, f); //Write valid units to file
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }


    IO_result = fwrite(&Building_stat.num_buildings, sizeof(Building_stat.num_buildings), 1, f); //Write number of buildings
    if (IO_result != 1)
    {
        fclose(f);
        return -2; //Write Error
    }


    if (Building_stat.num_buildings > 0)
    {

        fi = 0;
        di = 0;
        ti = 0;

        if (Building_stat.num_HQ > 0)
        {
            //First run for german HQ
            for (i = 0; i < Building_stat.num_buildings; i++)
            {
                if ((Building_info[i].Properties->Type == 0) && (Building_info[i].Properties->Owner == 0)) //german HQ
                {
                    IO_result = fwrite(Building_info[i].Properties,sizeof(Building_data), 1, f); //Write buildings contents
                    if (IO_result != 1)
                    {
                        fclose(f);
                        return -2; //Write Error
                    }
                    break;
                }
            }
            //Second run for french HQ
            for (i = 0; i < Building_stat.num_buildings; i++)
            {
                if ((Building_info[i].Properties->Type == 0) && (Building_info[i].Properties->Owner == 1)) //french HQ
                {
                    IO_result = fwrite(Building_info[i].Properties,sizeof(Building_data), 1, f); //Write buildings contents
                    if (IO_result != 1)
                    {
                        fclose(f);
                        return -2; //Write Error
                    }
                    break;
                }
            }
        }

        //run for factories

        if (Building_stat.num_F > 0)
        {
            for (offset = 0; offset < ((Map.width*Map.height)*2); offset+=2)
            {
                memcpy(&Field, Map.data + offset, sizeof(Field));

                if ((Field.Part >= 0x0C) && (Field.Part <= 0xE))
                {
                    index = Get_Buildings_info(offset/2);
                    if (index == -1)
                    {
                        QMessageBox              Errormsg;
                        Errormsg.warning(0,"Warning:","No data for factory "+QString::number(fi)+" found!");
                        Errormsg.setFixedSize(500,200);
                        //Try to fix the error
                        if (Field.Part = 0x0C)
                            Bdata.Owner = 2;  //neutral
                        if (Field.Part = 0x0D)
                            Bdata.Owner = 0;  //german
                        if (Field.Part = 0x0E)
                            Bdata.Owner = 1;  //french

                        Bdata.Type = 1;
                        Bdata.Index = fi;
                        Bdata.Resources = 0;
                        Bdata.Units[0] = 0xFF;
                        Bdata.Units[1] = 0xFF;
                        Bdata.Units[2] = 0xFF;
                        Bdata.Units[3] = 0xFF;
                        Bdata.Units[4] = 0xFF;
                        Bdata.Units[5] = 0xFF;
                        Bdata.Units[6] = 0xFF;
                        Bdata.Unknown = 0;
                        IO_result = fwrite(&Bdata,sizeof(Bdata), 1, f); //Write buildings contents
                    }
                    else
                    {
                        Building_info[index].Properties->Index = fi;
                        IO_result = fwrite(Building_info[index].Properties,sizeof(Building_data), 1, f); //Write buildings contents
                    }
                    if (IO_result != 1)
                    {
                    fclose(f);
                    return -2; //Write Error
                    }
                    fi++;
                }
            }
        }

        //run for depots

        if (Building_stat.num_D > 0)
        {
            for (offset = 0; offset < ((Map.width*Map.height)*2); offset+=2)
            {
                memcpy(&Field, Map.data + offset, sizeof(Field));
                if ((Field.Part >= 0x0F) && (Field.Part <= 0x11))
                {
                    index = Get_Buildings_info(offset/2);
                    if (index == -1)
                    {
                        QMessageBox              Errormsg;
                        Errormsg.warning(0,"Warning:","No data for depot "+QString::number(di)+" found!");
                        Errormsg.setFixedSize(500,200);

                        //Try to fix the error
                        if (Field.Part = 0x0F)
                            Bdata.Owner = 2;  //neutral
                        if (Field.Part = 0x10)
                            Bdata.Owner = 0;  //german
                        if (Field.Part = 0x11)
                            Bdata.Owner = 1;  //french

                        Bdata.Type = 2;
                        Bdata.Index = di;
                        Bdata.Resources = 0;
                        Bdata.Units[0] = 0xFF;
                        Bdata.Units[1] = 0xFF;
                        Bdata.Units[2] = 0xFF;
                        Bdata.Units[3] = 0xFF;
                        Bdata.Units[4] = 0xFF;
                        Bdata.Units[5] = 0xFF;
                        Bdata.Units[6] = 0xFF;
                        Bdata.Unknown = 0;
                        IO_result = fwrite(&Bdata,sizeof(Bdata), 1, f); //Write buildings contents
                    }
                    else
                    {
                        Building_info[index].Properties->Index = di;
                        IO_result = fwrite(Building_info[index].Properties,sizeof(Building_data), 1, f); //Write buildings contents
                    }

                    if (IO_result != 1)
                    {
                        fclose(f);
                        return -2; //Write Error
                    }
                    di++;
                }
            }
        }

        //run for transport units

        if (Building_stat.num_T > 0)
        {
            for (offset = 0; offset < ((Map.width*Map.height)*2); offset+=2)
            {
                memcpy(&Field, Map.data + offset, sizeof(Field));
                if ((Field.Unit == 0x2C) || (Field.Unit == 0x2D) ||
                    (Field.Unit == 0x34) || (Field.Unit == 0x35) ||
                    (Field.Unit == 0x3E) || (Field.Unit == 0x3F))
                {


                    index = Get_Buildings_info(offset/2);
                    if (index == -1)
                    {
                        QMessageBox              Errormsg;
                        Errormsg.warning(0,"Warning:","No data for transport unit "+QString::number(ti)+" found! Field : "+QString::number(offset/2)+" Unit :"+QString::number(Field.Unit));
                        Errormsg.setFixedSize(500,200);


                        //Try to fix the error
                        if ((Field.Unit == 0x2C) || (Field.Unit == 0x34) || (Field.Unit == 0x3E))
                            Bdata.Owner = 0;  //german
                        else
                            Bdata.Owner = 1;  //french

                        Bdata.Type = 3;
                        Bdata.Index = ti;
                        Bdata.Resources = 0;
                        Bdata.Units[0] = 0xFF;
                        Bdata.Units[1] = 0xFF;
                        Bdata.Units[2] = 0xFF;
                        Bdata.Units[3] = 0xFF;
                        Bdata.Units[4] = 0xFF;
                        Bdata.Units[5] = 0xFF;
                        Bdata.Units[6] = 0xFF;
                        Bdata.Unknown = 0;
                        IO_result = fwrite(&Bdata,sizeof(Bdata), 1, f); //Write buildings contents                                              

                    }
                    else
                    {
                        Building_info[index].Properties->Index = ti;
                        IO_result = fwrite(Building_info[index].Properties,sizeof(Building_data), 1, f); //Write buildings contents
                    }

                    if (IO_result != 1)
                    {
                        fclose(f);
                        return -2; //Write Error
                    }


                    ti++;
                }
            }
        }


    }

    fclose(f);

    return 0;
}




