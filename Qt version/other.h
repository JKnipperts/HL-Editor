

void Release_Buffers() //Memory cleanup...
{
    if (Map.data != NULL) free(Map.data);
    if (Partlib.data != NULL) free(Partlib.data);
    if (Unitlib.data != NULL) free(Unitlib.data);
    return;
}

bool Check_for_game_files()
{
    QFile PalFile(GameDir + Palette_name);
    QFile UnitdatFile(GameDir + Unitdat_name);
    QFile UnitlibFile(GameDir + Unitlib_name);
    QFile PartdatFile(GameDir + Partdat_name);
    QFile PartlibFile(GameDir + Partlib_name);



    if ((PalFile.exists() == false) ||
        (UnitdatFile.exists() == false) ||
        (UnitlibFile.exists() == false) ||
        (PartdatFile.exists() == false) ||
        (PartlibFile.exists() == false))
        return false;
    else
        return true;
}

bool Read_Config()
{
    QDir           dir;

    GameDir = "";

    QFile cfgFile(dir.currentPath()+Cfg);
    cfgFile.open(QIODevice::ReadOnly);

    if ((!cfgFile.exists()) || (!cfgFile.isOpen()))
    {
        return false;
    }
    else
    {
        QTextStream in(&cfgFile);
        in.readLineInto(&GameDir);
        cfgFile.read(Scale_factor);
        cfgFile.close();

        if (Check_for_game_files() == true)
            MapDir = GameDir+MapDir;

        if (Scale_factor == 0) Scale_factor = 2;
    }


    return true;
}


int Load_Ressources()
{
    QMessageBox              Errormsg;
    QString                  C_Filename1;
    QString                  C_Filename2;


    //Load Unit bitmaps
    C_Filename1 = (GameDir + Unitlib_name); //Create C style filenames for use of stdio
    C_Filename1.replace("/", "\\");
    C_Filename2 = (GameDir + Unitdat_name);
    C_Filename2.replace("/", "\\");

    if (Load_Unit_files(C_Filename1.toStdString().data(),C_Filename2.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error loading terrain files!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }

    //Load part/terrain bitmaps
    C_Filename1 = (GameDir + Partlib_name); //Create C style filenames for use of stdio
    C_Filename1.replace("/", "\\");
    C_Filename2 = (GameDir + Partdat_name);
    C_Filename2.replace("/", "\\");

    if (Load_Part_files(C_Filename1.toStdString().data(),C_Filename2.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error loading unit files!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }

    //Load color palette
    C_Filename1 = (GameDir + Palette_name); //Create a C style filename for use of stdio
    C_Filename1.replace("/'", "\\'");

    if (Load_Palette(C_Filename1.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error loading color palette!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }
    return 0;
}


int Load_Map()
{
    QMessageBox              Errormsg;
    QString                  C_Filename;

    C_Filename = Map_file;
    C_Filename.replace("/'", "\\'");

    if (Load_Mapdata(C_Filename.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error loading map data!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }
    return 0;
}



void ShowGrid()
// Draws a frame around each hex field to make them more visible
{

    if (Map.loaded == true)
    {
        int x,y,xp,yp;

        QPainter painter(&MapImageScaled);
        painter.setPen(QPen(Qt::white, 1));

        for (y = 0; y < Map.height; y++)
        {
            for (x = 0; x < Map.width; x++)
            {
                xp = x * (Tilesize-Tileshift);

                if (x % 2 != 0)
                    yp = (y * Tilesize) + (Tilesize / 2);
                else
                    yp = (y * Tilesize) ;

                xp = xp * Scale_factor;
                yp = yp * Scale_factor;

                painter.drawLine(xp,yp+((Tilesize/2)*Scale_factor),xp+(Tileshift*Scale_factor),yp);
                painter.drawLine(xp+(Tileshift*Scale_factor),yp,xp+((Tileshift*2)*Scale_factor),yp);
                painter.drawLine(xp+((Tileshift*2)*Scale_factor),yp,xp+(Tilesize*Scale_factor),yp+((Tilesize/2)*Scale_factor));
                painter.drawLine(xp+(Tilesize*Scale_factor),yp+((Tilesize/2)*Scale_factor),xp+((Tileshift*2)*Scale_factor),yp+(Tilesize*Scale_factor));
                painter.drawLine(xp+(Tileshift*Scale_factor),yp+(Tilesize*Scale_factor),xp+((Tileshift*2)*Scale_factor),yp+(Tilesize*Scale_factor));
                painter.drawLine(xp,yp+((Tilesize/2)*Scale_factor),xp+(Tileshift*Scale_factor),yp+(Tilesize*Scale_factor));
            }
        }

        painter.end();
    }

}

