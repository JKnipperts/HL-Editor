/* OTHER.H
 * Additional functions for the Hisotry Line Mapeditor. Draw hexagons, release memory, create child windows, etc.
*/

void Release_Buffers()
//Memory cleanup...
{
    if (Map.data != NULL) free(Map.data);
    if (Partlib.data != NULL) free(Partlib.data);
    if (Unitlib.data != NULL) free(Unitlib.data);
    if (Building_info != NULL) free(Building_info);
    if (SHP.buildings != NULL) free(SHP.buildings);

    return;
}

void Check_used_tiles()
{
    /*
     * Unfortunately, the game only uses a 64 KB buffer for the terrain graphics.
     * Since all terrain graphics together are well over 64 KB in size, this results in a limitation when using the graphics for your own maps.
     * In addition, the first 25 graphics are always loaded into the memory, while others are only loaded if they are used on a map.
     *
     * 25*594 (unpacked size of a terrain tile) = 14.850 byte
     * 65.535 byte (64 K buffer) - 14.850 = 50.686
     * 50.686 / 594 = 85
     * So this results in 85 mathematically possible additional parts the game can handle.
     *
     * During testing, however, errors sometimes occurred a little earlier (perhaps the game also uses the memory area for some variables or similar).
     * Therefore we give a warning from 80 parts.
     */

    if (show_warnings)
    {
        int upper_parts = 0;
        unsigned char used_parts[Num_Parts];
        memset(&used_parts,0,sizeof(used_parts));

        for (int offset = 0; offset < (Map.width*Map.height)*2; offset += 2) //Determine which tile graphics were used
        {
            if (Map.data[offset] != 0xAE)
            used_parts[Map.data[offset]] = 1;
        }

        for (int i = 0; i < Num_Parts; i++) //Check how many of these are extended graphics
        {
            if (used_parts[i] == 1)
            if (i > 24) upper_parts++;
        }

        if (upper_parts > 79)  //Issue a warning if 80 or more
        {
            QMessageBox              Warning;
            Warning.warning(0,"Warning:","You have used more extended terrain tiles (tiles outside of grass level and building parts) than can fit in the game's memory buffer. This can lead to graphic errors and incorrect display of the map in the game.");
            Warning.setFixedSize(500,200);
        }
    }
}



bool Check_for_game_files()
//Check whether game resources are available.
//For Linux compatibility, a separate check is also carried out for lower-case file names.
{

    QFile UnitdatFile(GameDir + Unitdat_name);
    QFile UnitlibFile(GameDir + Unitlib_name);
    QFile Partdat_S_File(GameDir + Partdat_S_name);
    QFile Partlib_S_File(GameDir + Partlib_S_name);
    QFile Partdat_W_File(GameDir + Partdat_W_name);
    QFile Partlib_W_File(GameDir + Partlib_W_name);
    QFile CodeFile = (GameDir + Code_name);

    QFile PalFile(GameDir + Palette_name);

    if (PalFile.exists() == false)
    {
        QFile PalFile(GameDir.toLower()+Palette_name.toLower());
        if (PalFile.exists() == false) return false;
    }
    if (UnitdatFile.exists() == false)
    {
        QFile UnitdatFile(GameDir.toLower()+Unitdat_name.toLower());
        if (UnitdatFile.exists() == false) return false;
    }
    if (UnitlibFile.exists() == false)
    {
        QFile UnitlibFile(GameDir.toLower()+Unitlib_name.toLower());
        if (UnitlibFile.exists() == false) return false;
    }
    if (Partdat_S_File.exists() == false)
    {
        QFile Partdat_S_File(GameDir.toLower()+Partdat_S_name.toLower());
        if (Partdat_S_File.exists() == false) return false;
    }
    if (Partlib_S_File.exists() == false)
    {
        QFile Partlib_S_File(GameDir.toLower()+Partlib_S_name.toLower());
        if (Partlib_S_File.exists() == false) return false;
    }
    if (Partdat_W_File.exists() == false)
    {
        QFile Partdat_W_File(GameDir.toLower()+Partdat_W_name.toLower());
        if (Partdat_W_File.exists() == false) return false;
    }
    if (Partlib_W_File.exists() == false)
    {
        QFile Partlib_W_File(GameDir.toLower()+Partlib_W_name.toLower());
        if (Partlib_W_File.exists() == false) return false;
    }
    if (CodeFile.exists() == false)
    {
        QFile CodeFile(GameDir.toLower()+Code_name.toLower());
        if (CodeFile.exists() == false) return false;
    }

        return true;
}


void Save()
//Save actual Mapdata
{
    if ((already_saved == false) || (Map_file == ""))
        Map_file = QFileDialog::getSaveFileName(0,"Save History Line 1914-1918 map file",MapDir,"HL map files (*.fin)");

    if (!Map_file.isEmpty() && !Map_file.isNull())
    {
        if ((!Map_file.contains(".fin")) && (!Map_file.contains(".FIN")))
         Map_file = Map_file+".FIN";


        if (Save_Mapdata(Map_file.toStdString().data()) != 0)
        {
            QMessageBox              Errormsg;
            Errormsg.warning(0,"","I cannot save the map data in "+Map_file);
            Errormsg.setFixedSize(500,200);
            return;
        }
        QString SHPfile;
        SHPfile = Map_file;
        SHPfile.replace(".fin",".shp").replace(".FIN",".SHP");

        if (Create_shp(SHPfile.toStdString().data()) != 0)
        {
            QMessageBox              Errormsg;
            Errormsg.warning(0,"","I cannot save the building data in "+SHPfile+"!");
            Errormsg.setFixedSize(500,200);
            return;
        }
        changes = false; //All changes saved
        already_saved = true;
    }
}



bool Read_Config()
//Reading the configuration file
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
//Loading game resources
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
        Errormsg.critical(0,"Error","Error loading unit files!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }


    //Load part/terrain bitmaps

    if (summer == false)  //it's winter
    {
        C_Filename1 = (GameDir + Partlib_W_name); //Create C style filenames for use of stdio
        C_Filename1.replace("/", "\\");
        C_Filename2 = (GameDir + Partdat_W_name);
        C_Filename2.replace("/", "\\");
    }
    else  //it's summer
    {
        C_Filename1 = (GameDir + Partlib_S_name); //Create C style filenames for use of stdio
        C_Filename1.replace("/", "\\");
        C_Filename2 = (GameDir + Partdat_S_name);
        C_Filename2.replace("/", "\\");
    }

    if (Load_Part_files(C_Filename1.toStdString().data(),C_Filename2.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error loading terrain files!");
        Errormsg.setFixedSize(500,200);
        if (Partlib.data != NULL) free(Partlib.data);
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


    //Load levelcodes
    C_Filename1 = (GameDir + Code_name); //Create a C style filename for use of stdio
    C_Filename1.replace("/'", "\\'");
    if (Get_Levelcodes(C_Filename1.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error reading levelcodes!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }

    //Read unit names
    C_Filename1 = (GameDir + Unitdat2_name); //Create a C style filename for use of stdio
    C_Filename1.replace("/'", "\\'");
    if (Get_unit_data(C_Filename1.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Error reading unit data!");
        Errormsg.setFixedSize(500,200);
        Release_Buffers();
        return -1;
    }
    Res_loaded = true;
    return 0;
}


int Load_Map()
//Load a map file
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

    C_Filename.replace(".fin",".shp").replace(".FIN",".SHP");


    if (Read_shp_data(C_Filename.toStdString().data()) != 0)
    {
        Errormsg.warning(0,"Warning:","Cannot find building data for this map. "+C_Filename+"! corrupted or missing?");
        Errormsg.setFixedSize(500,200);
        memset(SHP.can_be_built,0,sizeof(SHP.can_be_built));
        Building_info = NULL;

        Create_building_record_from_map();
    }
    else
        Add_building_positions();

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

        for (y = 0; y < (Map.height-1); y++)
        {
            for (x = 0; x < (Map.width-1); x++)
            {
                if (summer)
                Draw_Hexagon(x,y,QPen(Qt::white, 1),&MapImageScaled,true,true);
                else
                Draw_Hexagon(x,y,QPen(Qt::black, 1),&MapImageScaled,true,true);
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
        tile_selection->setWindowFlags(Qt::WindowStaysOnTopHint);
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
        Draw_Hexagon(0,0,QPen(Qt::red, 1),&TileListImageScaled,false,true);
        selected_tile = 0;
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
        unit_selection ->setWindowFlags(Qt::WindowStaysOnTopHint);

        unit_selection->resize(((11*Tilesize)*Scale_factor)+10, (2*((Num_Units/10)+1)*Tilesize)*Scale_factor);
        unit_selection->setWindowTitle("Unit selection");
        unit_selection->setMouseTracking(true);

        unit_name_text = new QLabel();
        unit_name_text->setText("");


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
        label->setPixmap(QPixmap::fromImage(UnitListImageScaled));

        unitscrollArea = new(QScrollArea);
        unitscrollArea->setBackgroundRole(QPalette::Dark);
        unitscrollArea->setWidget(label);
        unitscrollArea->setVisible(true);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(unitscrollArea);
        layout->addWidget(unit_name_text);

        unit_selection->setLayout(layout);
        unit_selection->move(screenrect.width()/2, screenrect.bottom()/2);

        unit_selection->show();
    }
}


void Create_buildable_units_window()
{

    buildable = new buildablewindow();
    buildable->setWindowFlag(Qt::SubWindow);
    buildable->setWindowFlags(Qt::WindowStaysOnTopHint);
    buildable->resize(((11*Tilesize)*Scale_factor)+10, (((Num_Units/10)+1)*Tilesize)*Scale_factor);
    buildable->setWindowTitle("Units buildable in factories");

        BuildableImage = QImage((10*Tilesize),((Num_Units/10)+1)*Tilesize, QImage::Format_RGB16); //Create a new QImage object
        BuildableImage.fill(Qt::transparent);


        int tx = 0;
        int ty = 0;

        for (int tc = 0; tc < Num_Units; tc++)
        {
        if (SHP.can_be_built[tc] == 0)
            Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,1,&BuildableImage);
        else
            Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,3,&BuildableImage);


        tx++;
        if (tx == 10)
        {
            tx = 0;
            ty++;
        }
        }


        BuildableImageScaled = BuildableImage.scaled(BuildableImage.width()*Scale_factor,BuildableImage.height()*Scale_factor); //Create a scaled version of it


        QLabel *label = new QLabel();
        QHBoxLayout *layout = new QHBoxLayout();
        label->setPixmap(QPixmap::fromImage(BuildableImageScaled));

        buildablescrollArea = new(QScrollArea);
        buildablescrollArea->setBackgroundRole(QPalette::Dark);
        buildablescrollArea->setWidget(label);
        buildablescrollArea->setVisible(true);
        layout->addWidget(buildablescrollArea);
        buildable->setLayout(layout);

        buildable->show();

}


void Create_building_configuration_window()
{

        if (selected_building == -1)
        {
            QMessageBox              Errormsg;
            Errormsg.critical(0,"Error","There is no data record for this building!");
            Errormsg.setFixedSize(500,200);
            return;
        }
        else
        {
            building_window = new buildingwindow();
            building_window->setWindowFlag(Qt::SubWindow);
            building_window->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);

            QString Building_title = "Properties of ";
            switch (Building_info[selected_building].Properties->Owner)
            {
                case 0:
                Building_title += "German ";
                break;
                case 1:
                Building_title += "French ";
                break;
                case 2:
                Building_title += "Neutral ";
                break;
            }

            switch (Building_info[selected_building].Properties->Type)
            {
                case 0:
                Building_title += "Headquarter";
                break;
                case 1:
                Building_title += "Factory";
                break;
                case 2:
                Building_title += "Depot ";
                break;
                case 3:
                Building_title += "Transport Unit ";
                break;
            }



            building_window->setWindowTitle(Building_title);
            Building_Image = QImage((7*Tilesize),Tilesize+1, QImage::Format_RGB16); //Create a new QImage object
            Building_Image.fill(Qt::transparent);


            for (int i = 0; i < 7; i++)
            {
                    if (Building_info[selected_building].Properties->Units[i] != 0xFF)
                    Draw_Unit(i * Tilesize, 0,(Building_info[selected_building].Properties->Units[i] * 6) , Building_info[selected_building].Properties->Owner+1, &Building_Image);
            }

            Building_Image_Scaled = Building_Image.scaled(Building_Image.width()*Scale_factor,Building_Image.height()*Scale_factor); //Create a scaled version of it

            for (int i = 0; i < 7; i++)
            {
                    QPainter painter(&Building_Image_Scaled);
                    QPen pen;
                    pen.setWidth(1);
                    pen.setColor(Qt::white);
                    painter.setPen(pen);
                    QRect R((i*Tilesize)*Scale_factor,0,((i*Tilesize)+Tilesize)*Scale_factor,Building_Image_Scaled.height()-1);
                    painter.drawRect(R);
                    painter.end();
            }
            QLabel *textlabel1 = new QLabel();
            textlabel1->setText("Units in the building:");
            QLabel *textlabel2 = new QLabel();
            textlabel2->setText("Resources generated by this building per turn:");

            QLabel *bitmaplabel = new QLabel();
            bitmaplabel->setPixmap(QPixmap::fromImage(Building_Image_Scaled));

            Building_ScrollArea = new(QScrollArea);
            Building_ScrollArea->setBackgroundRole(QPalette::Dark);
            Building_ScrollArea->setWidget(bitmaplabel);
            Building_ScrollArea->resize(Building_Image_Scaled.width(),Building_Image_Scaled.height());
            Building_ScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            Building_ScrollArea->verticalScrollBar()->hide();
            Building_ScrollArea->verticalScrollBar()->resize(0, 0);
            Building_ScrollArea->setVisible(true);


            RessourceEdit = new QLineEdit;
            RessourceEdit->clear();
            RessourceEdit->setValidator( new QIntValidator(0, 255) );
            RessourceEdit->setText(QString::number(Building_info[selected_building].Properties->Resources));  //Bisherigen wert anzeigen


            QVBoxLayout *layout = new QVBoxLayout();
            layout->addWidget(textlabel1);
            layout->addWidget(Building_ScrollArea);
            layout->addWidget(textlabel2);
            layout->addWidget(RessourceEdit);

            building_window->setLayout(layout);
            building_window->show();
        }
}



void Create_replace_tile_diag()
{

        replace_accepted = false;
        r1 = selected_tile;
        r2 = 0;

        replacedlg = new replacewindow();
        replacedlg->setWindowFlag(Qt::SubWindow);
        replacedlg->setWindowFlags(Qt::WindowStaysOnTopHint);
        replacedlg->resize(((2*Tilesize)*Scale_factor)+20, Tilesize*Scale_factor);
        replacedlg->setWindowTitle("Replace tiles");

        QLabel *textlabel1 = new QLabel();
        textlabel1->setText("Replace");
        QLabel *textlabel2 = new QLabel();
        textlabel2->setText("with");

        tile_image1 = QImage(Tilesize,Tilesize, QImage::Format_RGB16); //Create a new QImage object
        tile_image1.fill(Qt::transparent);
        Draw_Part(0,0,r1,&tile_image1);
        tile_image2 = QImage(Tilesize,Tilesize, QImage::Format_RGB16); //Create a new QImage object
        tile_image2.fill(Qt::transparent);
        Draw_Part(0,0,r2,&tile_image2);

        tile_image1 = tile_image1.scaled(tile_image1.width()*Scale_factor,tile_image1.height()*Scale_factor); //scale it
        tile_image2 = tile_image2.scaled(tile_image2.width()*Scale_factor,tile_image2.height()*Scale_factor); //scale it

        Tile1 = new QLabel();
        Tile1->setPixmap(QPixmap::fromImage(tile_image1));
        Tile2 = new QLabel();
        Tile2->setPixmap(QPixmap::fromImage(tile_image2));

        ok_button = new QPushButton("OK");
        QObject::connect(ok_button, &QPushButton::clicked, [=]()
        {
            replace_accepted = true;
            replacedlg->close();
        });

        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(textlabel1);
        layout->addWidget(Tile1);
        layout->addWidget(textlabel2);
        layout->addWidget(Tile2);
        layout->addWidget(ok_button);

        replacedlg->setLayout(layout);
        replacedlg->show();
}

