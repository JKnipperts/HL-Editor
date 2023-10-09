

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
       // Release_Buffers();
        return -1;
    }
    return 0;
}


void Draw_Hexagon(int x, int y,QPen Pen, QImage *Image, bool align, bool scaling)
{
    int xp,yp;
    QPainter painter(Image);
    painter.setPen(Pen);



    if (align)
    {
        xp = x * (Tilesize-Tileshift);
        if (x % 2 != 0)
            yp = (y * Tilesize) + (Tilesize / 2);
        else
            yp = (y * Tilesize) ;
    }
    else
    {
         xp = (x * Tilesize);
         yp = (y * Tilesize);
    }


    if (scaling)
    {
        xp = xp * Scale_factor;
        yp = yp * Scale_factor;

        painter.drawLine(xp,yp+((Tilesize/2)*Scale_factor),xp+(Tileshift*Scale_factor),yp);
        painter.drawLine(xp+(Tileshift*Scale_factor),yp,xp+((Tileshift*2)*Scale_factor),yp);
        painter.drawLine(xp+((Tileshift*2)*Scale_factor),yp,xp+(Tilesize*Scale_factor),yp+((Tilesize/2)*Scale_factor));
        painter.drawLine(xp+(Tilesize*Scale_factor),yp+((Tilesize/2)*Scale_factor),xp+((Tileshift*2)*Scale_factor),yp+(Tilesize*Scale_factor));
        painter.drawLine(xp+(Tileshift*Scale_factor),yp+(Tilesize*Scale_factor),xp+((Tileshift*2)*Scale_factor),yp+(Tilesize*Scale_factor));
        painter.drawLine(xp,yp+((Tilesize/2)*Scale_factor),xp+(Tileshift*Scale_factor),yp+(Tilesize*Scale_factor));
    }
    else
    {
        painter.drawLine(xp,yp+(Tilesize/2),xp+Tileshift,yp);
        painter.drawLine(xp+Tileshift,yp,xp+(Tileshift*2),yp);
        painter.drawLine(xp+(Tileshift*2),yp,xp+Tilesize,yp+(Tilesize/2));
        painter.drawLine(xp+Tilesize,yp+(Tilesize/2),xp+(Tileshift*2),yp+Tilesize);
        painter.drawLine(xp+Tileshift,yp+Tilesize,xp+(Tileshift*2),yp+Tilesize);
        painter.drawLine(xp,yp+(Tilesize/2),xp+Tileshift,yp+Tilesize);
    }


    painter.end();
}


void Redraw_Field(int x, int y,int part, int unit)
{
    int xp,yp,side;

    xp = x * (Tilesize-Tileshift);

    if (x % 2 != 0)
        yp = (y * Tilesize) + (Tilesize / 2);
    else
        yp = (y * Tilesize) ;

    Draw_Part(xp,yp,part,&MapImage);

    if (unit != 0xFF)
    {
        if (unit % 2 == 0) side = 1; else side = 2;
        unit = (unit / 2) * 6;
        if (side == 1) unit = unit + 3;
        Draw_Unit(xp,yp, unit, side, &MapImage);
    }
}


void ShowGrid()
// Draws a frame around each hex field to make them more visible
{

    if (Map.loaded == true)
    {
        int x,y;

        for (y = 0; y < Map.height; y++)
        {
            for (x = 0; x < Map.width; x++)
            {
                Draw_Hexagon(x,y,QPen(Qt::white, 1),&MapImageScaled,true,true);
            }
        }    
    }

}


void Create_Tileselection_window()
{
    if (Map.loaded == true)
    {
        tile_selection = new tilelistwindow();
        tile_selection->setWindowFlag(Qt::SubWindow);
        tile_selection->resize(((11*Tilesize)*Scale_factor)+10, ((11*Tilesize)*Scale_factor));
        tile_selection->setWindowTitle("Tile selection");

        TileListImage = QImage((10*Tilesize),((Num_Parts/10)+1)*Tilesize, QImage::Format_RGB16); //Create a new QImage object for the tile list
        TileListImage.fill(Qt::transparent);

        int tx = 0;
        int ty = 0;

        for (int tc = 0; tc < Num_Parts; tc++)
        {
            Draw_Part(tx*Tilesize,ty*Tilesize,tc,&TileListImage); //Draw the bitmap
            tx++;
            if (tx == 10)
            {
                tx = 0;
                ty++;
            }
        }

        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Create a scaled version of it

        QLabel *label = new QLabel();
        QHBoxLayout *layout = new QHBoxLayout();
        label->setPixmap(QPixmap::fromImage(TileListImageScaled));

        tilescrollArea = new(QScrollArea);
        tilescrollArea->setBackgroundRole(QPalette::Dark);
        tilescrollArea->setWidget(label);
        tilescrollArea->setVisible(true);
        layout->addWidget(tilescrollArea);
        tile_selection->setLayout(layout);
        tile_selection->move(screenrect.width()/2, screenrect.top());
        tile_selection->show();
    }
}


void Create_Unitselection_window()
{
    if (Map.loaded == true)
    {
        unit_selection = new unitlistwindow();
        unit_selection->setWindowFlag(Qt::SubWindow);
        unit_selection->resize(((11*Tilesize)*Scale_factor)+10, (2*((Num_Units/10)+1)*Tilesize)*Scale_factor);
        unit_selection->setWindowTitle("Unit selection");

        UnitListImage = QImage((10*Tilesize),2*((Num_Units/10)+1)*Tilesize, QImage::Format_RGB16); //Create a new QImage object for the tile list
        UnitListImage.fill(Qt::transparent);


        int tx = 0;
        int ty = 0;

        for (int tc = 0; tc < Num_Units; tc++)
        {
            Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,1,&UnitListImage);

            tx++;
            if (tx == 10)
            {
                tx = 0;
                ty++;
            }
        }
        ty++;
        tx=0;
        for (int tc = 0; tc < Num_Units; tc++)
        {
            Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,2,&UnitListImage);
             tx++;
            if (tx == 10)
            {
                tx = 0;
                ty++;
            }
        }

        UnitListImageScaled = UnitListImage.scaled(UnitListImage.width()*Scale_factor,UnitListImage.height()*Scale_factor); //Create a scaled version of it

        QLabel *label = new QLabel();
        QHBoxLayout *layout = new QHBoxLayout();
        label->setPixmap(QPixmap::fromImage(UnitListImageScaled));

        unitscrollArea = new(QScrollArea);
        unitscrollArea->setBackgroundRole(QPalette::Dark);
        unitscrollArea->setWidget(label);
        unitscrollArea->setVisible(true);
        layout->addWidget(unitscrollArea);
        unit_selection->setLayout(layout);

        unit_selection->move(screenrect.width()/2, screenrect.bottom()/2);

        unit_selection->show();
    }
}
