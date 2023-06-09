/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21
#define PRIORITY_TSTARTCAM 20
#define PRIORITY_TUPDATEBATTERY 24
#define PRIORITY_TSTOPCAM 26
#define PRIORITY_CAPTIMG 24
#define PRIORITY_INITARENA 26
#define PRIORITY_POSITION 19

Camera * camera;

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;
    
    camera = new Camera(sm,5);

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
      if (err = rt_mutex_create(&mutex_cam, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_arena, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    

    
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobotWithWD, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_watchdog, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_sem_create(&sem_getBattery, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startCam, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
      if (err = rt_sem_create(&sem_CaptImg, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_sem_create(&sem_closeCam, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_InitArena, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_position, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_ArenaOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobotWithWD, "th_startRobotWtihWD", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_update_battery, "th_update_battery", 0, PRIORITY_TUPDATEBATTERY, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_startCam, "th_startCam", 0, PRIORITY_TSTARTCAM, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);

    }
    if (err = rt_task_create(&th_CaptImg, "th_CaptImg", 0, PRIORITY_CAPTIMG, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }    
      if (err = rt_task_create(&th_closeCam, "th_closeCam", 0, PRIORITY_TSTOPCAM, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }  

    if (err = rt_task_create(&th_InitArena, "th_InitArena", 0, PRIORITY_INITARENA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }  
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_task_create(&th_Position, "th_Position", 0, PRIORITY_POSITION, 0))
    {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobotWithWD, (void(*)(void*)) & Tasks::StartRobotTaskWithWD, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_update_battery, (void(*)(void*)) & Tasks::UpdateBattery, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startCam, (void(*)(void*)) & Tasks::startCam, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_CaptImg, (void(*)(void*)) & Tasks::CaptImg, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
      if (err = rt_task_start(&th_closeCam, (void(*)(void*)) & Tasks::closeCam, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_InitArena, (void(*)(void*)) & Tasks::InitArena, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_task_start(&th_Position, (void(*)(void*)) & Tasks::PositionRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
         exit(EXIT_FAILURE);
    }
    
    


    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            // APPELER FONCTION 5
            MonitorError(msgRcv) ;
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {
            rt_sem_v(&sem_startRobotWithWD);
        } else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)){
            rt_sem_v(&sem_startCam);
        } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)){
            rt_sem_v(&sem_closeCam);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_BATTERY_GET)){
            rt_sem_v(&sem_getBattery);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA)){
            rt_sem_v(&sem_InitArena);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_CONFIRM)){
            rt_sem_v(&sem_ArenaOk);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_INFIRM)){
            rt_sem_v(&sem_ArenaOk);
        }
         else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_START)) {
            rt_sem_v(&sem_position);
        }
        else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_STOP))
        {
            positionActivated = false;
        }
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    

    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;
        
        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {

        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        cout << "Start robot without watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(robot.StartWithoutWD());
        // appeler ComptError
        ComptorError(msgSend) ;
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;
        
        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
        
    }
}


//Fonctionnalité 11
void Tasks::StartRobotTaskWithWD(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    while (1) {
        
        int rs;

        Message * msgSend;
        rt_sem_p(&sem_startRobotWithWD, TM_INFINITE);
        cout << "Start robot with watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(robot.StartWithWD());
        ComptorError(msgSend) ;
        rt_mutex_release(&mutex_robot);
        
        //rt_sem_v(&sem_watchdog) ;
        
        cout << msgSend->GetID();
        cout << ")" << endl;
        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        
        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
            rt_task_set_periodic(NULL, TM_NOW, 1000000000);
            
            while(1) {
                rt_task_wait_period(NULL);
                rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                rs = robotStarted ;
                rt_mutex_release(&mutex_robotStarted) ;
                if(rs == 1) {
                    cout << "Periodic Reload update"<< endl << flush ;
                    rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                    robot.Write(robot.ReloadWD());
                    rt_mutex_release(&mutex_robot) ;
                } else {
                    break ;
                }
                
            }
            
            
        }
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon
        
    }
}



/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            Message * msg ;
            msg = new Message((MessageID)cpMove) ;
            Message * msgSend ;
            msgSend = robot.Write(msg);
            ComptorError(msgSend) ;
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/*else {
        cout << "@msg :" << msg << endl << flush;
    } */

    return msg;
}

//Fonctionnalité 5-6
void Tasks::MonitorError(Message * msgReceived) {
    
    if (msgReceived->GetID() == MESSAGE_MONITOR_LOST) {
        
        cout << " !!! Communication Lost between monitor and supervisor " << __PRETTY_FUNCTION__ << endl << flush;
        delete(msgReceived);
        robot.Stop() ; // arret du robot
        closeCam(); // fermeture de la caméra
        monitor.AcceptClient();
    }
    else {
        cout << " **** F5 OK " << __PRETTY_FUNCTION__ << endl << flush;
    }

}

//Fonctionnalité 8 - 9
void Tasks::ComptorError(Message * msgSend) {
    
    if ( (msgSend->GetID() == MESSAGE_ANSWER_COM_ERROR) || (msgSend->GetID() == MESSAGE_ANSWER_ROBOT_ERROR) || (msgSend->GetID() == MESSAGE_ANSWER_ROBOT_TIMEOUT) ){
        count = count +1;
         cout << " Error +1 " << __PRETTY_FUNCTION__ << endl << flush;
    }
    else {
        count = 0;
    }
    if (count >3) {
        msgSend = new Message(MESSAGE_MONITOR_LOST);
        cout << "!!!! Connection lost (3 errors) !!!!" << __PRETTY_FUNCTION__ << endl << flush;
        //fermer com robot - superviseur
        cout << " => Close Communication <= " << __PRETTY_FUNCTION__ << endl << flush;
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        robotStarted = 0 ;
        rt_mutex_release(&mutex_robotStarted);
        
        robot.Close();
        //remettre dans état initial
        rt_sem_v(&sem_openComRobot);
        //OpenComRobot();
    }

}



/**
 * @brief Thread handling update of battery of the robot.
 */
//Fonctionnalité 13
void Tasks::UpdateBattery() {
    rt_task_set_periodic(NULL, TM_NOW, 500000000);
    
    MessageBattery *msg;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;

    rt_sem_p(&sem_barrier, TM_INFINITE);
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_getBattery, TM_INFINITE);

    while (1) {
        rt_task_wait_period(NULL);
        
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msg = (MessageBattery*)robot.Write(new Message(MESSAGE_ROBOT_BATTERY_GET)); 
        cout <<  "---------------" << __PRETTY_FUNCTION__ << endl << flush;
        ComptorError(msg) ;
        rt_mutex_release(&mutex_robot);
        
        WriteInQueue(&q_messageToMon, msg);

    }
   
}

//Fonctionnalité 14
void Tasks::startCam() {
    Message *msgSend;
    cout << "StartING start cam " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);
    while (1) {
        rt_sem_p(&sem_startCam, TM_INFINITE);
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        camera -> Open();
         cout << "HEYOOOOO " << __PRETTY_FUNCTION__ << endl << flush;
        if(camera -> IsOpen()){
            cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
            rt_sem_v(&sem_CaptImg);
            rt_mutex_release(&mutex_cam);
             //   WriteInQueue(&q_messageToMon,msgImg);
        } else {
            rt_mutex_release(&mutex_cam);
             cout << "not working" << __PRETTY_FUNCTION__ << endl << flush;
            msgSend = new Message(MESSAGE_ANSWER_NACK);
            WriteInQueue(&q_messageToMon, msgSend);

        }
    }
}  

//Fonctionnalité 15
void Tasks::CaptImg() {
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
 
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);
    // Synchronization barrier (waiting that all tasks are starting)
    
    rt_sem_p(&sem_CaptImg, TM_INFINITE);
   
    while (1) {
        //cout << "captimg " << __PRETTY_FUNCTION__ << endl << flush;
        rt_task_wait_period(NULL);
             // Synchronization barrier (waiting that all tasks are starting)
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        if(camera -> IsOpen()){
            Img * img = new Img(camera->Grab());
            MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
            WriteInQueue(&q_messageToMon, msgImg);
            
        } else {
            cout << "captimg not working" << __PRETTY_FUNCTION__ << endl << flush;
        }
         
        rt_mutex_release(&mutex_cam);
       
        
    }
}
    
//Fonctionnalité 16
void Tasks::closeCam() {

    Message *msgSend;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;

    rt_sem_p(&sem_barrier, TM_INFINITE);
    while (1) {
        rt_sem_p(&sem_closeCam, TM_INFINITE);
        cout << "cam closing " << __PRETTY_FUNCTION__ << endl << flush;

        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        if(camera -> IsOpen()){
            camera -> Close();
        } else {
            cout << "cam closing not working" << __PRETTY_FUNCTION__ << endl << flush;

        }
        rt_mutex_release(&mutex_cam);

    }

}



//Fonctionnalité 17
   void Tasks::InitArena() {
       
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
   while (1) {
       
        rt_task_wait_period(NULL);
         cout << "BEGINNN " << __PRETTY_FUNCTION__ << endl << flush;
        
        rt_sem_p(&sem_InitArena, TM_INFINITE);
        
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        Img * img = new Img(camera->Grab());
        
        rt_mutex_acquire(&mutex_arena, TM_INFINITE);
        arena = img -> SearchArena();
        rt_mutex_release(&mutex_arena);

         cout << "ENDDD " << __PRETTY_FUNCTION__ << endl << flush;
         
        if (arena.IsEmpty()) {
            cout << "Empty arena" << __PRETTY_FUNCTION__ << endl << flush;
            rt_mutex_release(&mutex_cam);
        } else {
             cout << "ARENAAA DRAWING " << __PRETTY_FUNCTION__ << endl << flush;
            img -> DrawArena(arena);
            
            MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
            WriteInQueue(&q_messageToMon, msgImg);
            
            cout << "ARENA OK " << __PRETTY_FUNCTION__ << endl << flush;
            
            rt_sem_p(&sem_ArenaOk, TM_INFINITE);
            rt_mutex_release(&mutex_cam);
            //wait for semaphore to validate arena

        }
 
      
    }
   }
 void Tasks::PositionRobot(void *arg){
    MessagePosition *msgPos;
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
     rt_sem_p(&sem_barrier, TM_INFINITE);
    while(1)    {
          
        rt_sem_p(&sem_position, TM_INFINITE);
        
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        
        positionActivated = true;
        
        while(positionActivated){
            Img * img = new Img(camera->Grab());
            std::list<Position>  positions = img->SearchRobot(arena);
             img->DrawRobot(positions.front());
                msgPos = new MessagePosition(MESSAGE_CAM_POSITION, positions.front());
                cout << "Positions Not Found" << endl << flush;
       
            MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
            WriteInQueue(&q_messageToMon, msgImg);
            WriteInQueue(&q_messageToMon, msgPos);
            
        }
        
        rt_mutex_release(&mutex_cam);
    }
    
}

   
