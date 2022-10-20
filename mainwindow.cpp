#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{


    ui->setupUi(this);
    nDate = QDate::currentDate();

    strDate = QDate::currentDate().toString("yyyy/M/dd");   //print current day
    ui->label_2->setText(strDate);
    if (DateMap[QDate::currentDate().toString("yyyy/M/dd")].isReserved == false)
    {
        ui->label_3->setText("Term possible for reservation");
    }
    else { ui->label_3->setText("Term was reserved");}

    QDir cat;
    FilePath=cat.currentPath();//take current directory

    //sms
    ui->lineEdit->setText(strDate +" I'm booking" );
    SmsR.smstext=ui->lineEdit->text();
    //tcp
    client = new ClientStuff("localhost", 6547);//ip and port server

    setStatus(client->getStatus());
    connect(client, &ClientStuff::hasReadSome, this, &MainWindow::receivedSomething);//if server sent something
    connect(client, &ClientStuff::statusChanged, this, &MainWindow::setStatus);//if server change the status
    connect(client->tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
          this, SLOT(gotError(QAbstractSocket::SocketError)));
}

MainWindow::~MainWindow()
{

    delete ui;
    delete client;//tcp
}

void MainWindow::on_calendarWidget_selectionChanged()
{

   ui->label_4->setText("");
   nDate = ui->calendarWidget->selectedDate();//nDate for selected on calender day
   strDate = nDate.toString("yyyy/M/dd");

   if (!(DateMap.contains(strDate)) || (DateMap[strDate].isReserved==false))
   {
           ui->label_3->setText("the term possible for reservation");
   }
   else { ui->label_3->setText("the term was reserved");}

   QMapIterator<QString,rdata>Iter(DateMap);
   while(Iter.hasNext())
   {
       Iter.next();
       qDebug()<<Iter.key()<<" "<<Iter.value().isReserved;
   }

   ui->label_2->setText(strDate);
   client->send_to_server("isfree@" + strDate);
}

void MainWindow::on_pushButton_clicked()
{

    if (DateMap[strDate].isReserved == false)
    {
        DateMap[strDate].isReserved = true;
        ui->label_4->setText("You reserved this date");
    }
    else { ui->label_4->setText("You cannot reserve again");}

   client->send_to_server("reserve@" + strDate);

}

void MainWindow::on_pushButton_2_clicked()
{
    if (DateMap[strDate].isReserved == true)
    {
      DateMap[strDate].isReserved = false;
      ui->label_4->setText("You canceled this term");
    }
    else { ui->label_4->setText("You don't have a reservation yet");}
    client->send_to_server("release@" + strDate);
}

void MainWindow::on_commandLinkButton_clicked()//button with months
{

    arrowDate=nDate.addMonths(1);//one month later
    ui->calendarWidget->setSelectedDate(arrowDate);
    QString strMonth = arrowDate.toString("M");
    bool ok;
    int k = strMonth.toInt(&ok);
    if (ok == false) {qDebug() << "not converted";}
    strMonth = QDate::longMonthName(k);

}
void MainWindow::on_commandLinkButton_2_clicked()
{

    arrowDate=nDate.addMonths(-1);//one month earlier
    ui->calendarWidget->setSelectedDate(arrowDate);
    QString strMonth = arrowDate.toString("M");
    bool ok;
    int k = strMonth.toInt(&ok);
    if (ok == false) {qDebug() << "not converted";}
    strMonth = QDate::longMonthName(k);


}


void MainWindow::on_pushButton_4_clicked()
{

 SmsR.smstext = ui->lineEdit->text();
 QString strDate = SmsR.smstext.left(9);
 QString Rezor = SmsR.smstext.mid(10,17);
 if(Rezor == "rezerwuje")
 {

     if (DateMap[strDate].isReserved == false)
     {
         DateMap[strDate].isReserved = true;
         ui->label_4->setText("You reserved the term");
     }
     else { ui->label_4->setText("You cannot reserve again");}
     client->send_to_server("reserve@" + strDate);
 }
 if(Rezor == "odwołuje")
 {
     if (DateMap[strDate].isReserved == true)
     {
       DateMap[strDate].isReserved = false;
       ui->label_4->setText("you canceled this term");
     }
     else { ui->label_4->setText("You don't have reservation' ");}
     client->send_to_server("release@" + strDate);
 }
 qDebug()<<strDate<<Rezor;
}


//tcp
void MainWindow::setStatus(bool newStatus)
{
    if(newStatus)
    {
        ui->label_status->setText(
                    tr("<font color=\"green\">CONNECTED</font>"));
        ui->pushButton_connect->setVisible(false);
        ui->pushButton_disconnect->setVisible(true);
    }
    else
    {
        ui->label_status->setText(
                  tr("<font color=\"red\">DISCONNECTED</font>"));
        ui->pushButton_connect->setVisible(true);
        ui->pushButton_disconnect->setVisible(false);
    }
}

void MainWindow::receivedSomething(QString Msgs)
{
    ui->textEdit_log->append("I recived: "+Msgs);
    qDebug()<<Msgs<<"::";

   QString Order = "", Date = "";
   bool NextInfo = false;
   for(int i = 0;i < Msgs.length(); i++)
   {
        if (Msgs[i] != '@' && !NextInfo)
        {
           Order += Msgs[i];
           continue;
        }
        if(Msgs[i] == '@')
        {
            NextInfo = true;
            continue;
        }
        Date += Msgs[i];
    }
        qDebug()<<Order<<Date;


   if(Order == "notreserved")
   {
       ui->label_3->setText("Server says: the term possible for reservation");
       ui->label_2->setText(Date);
   }
   if(Order == "isreserved" )
   {
       ui->label_3->setText("Server says: the term was reserved ");
       ui->label_2->setText(Date);
   }

   if(Order == "Ireserved")
   {
       ui->label_3->setText("Server says: you reserved the term");
       ui->label_2->setText(Date);
   }
   if(Order == "Inotreserved" )
   {
       ui->label_3->setText("Server says: The Term has been reserved befor");
       ui->label_2->setText(Date);
   }

   if(Order == "isreleased")
   {
       ui->label_3->setText("Server mowi: Odwołałes ten termin");
       ui->label_2->setText(Date);
   }
   if(Order == "isnotreleased" )
   {
       ui->label_3->setText("Server mowi: Nie było rezerwacji");
       ui->label_2->setText(Date);//add during a time reserved
   }


   if (Order != "notreserved" && Order != "isreserved"
           && Order != "Ireserved" && Order != "Inotreserved"
           && Order!= "isnotreleased" && Order != "isreleased")
       qDebug()<<"don't now answer";

}

void MainWindow::gotError(QAbstractSocket::SocketError err)
{
    //qDebug() << "got error";
    QString strError = "unknown";
    switch (err)
    {
        case 0:
            strError = "Connection was refused";
            break;
        case 1:
            strError = "Remote host closed the connection";
            break;
        case 2:
            strError = "Host address was not found";
            break;
        case 5:
            strError = "Connection timed out";
            break;
        default:
            strError = "Unknown error";
    }

    ui->textEdit_log->append(strError);
}


void MainWindow::on_pushButton_connect_clicked()
{
     client->connect2host();
}

void MainWindow::on_pushButton_disconnect_clicked()
{
    client->closeConnection();
}


void MainWindow::on_pushButton_send_clicked()
{
    QString as = "as";
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    //out.setVersion(QDataStream::Qt_5_10);
    out << quint16(0) << ui->lineEdit_message->text();

    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    client->tcpSocket->write(arrBlock);

}



