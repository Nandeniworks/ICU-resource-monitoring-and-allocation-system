#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <limits>
#include <ctime>
using namespace std;

static string nowTime()
{
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    return string(buf);
}

static void clearLine()
{
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


class Patient
{
public:
    int id = 0;
    string name;
    string diagnosis;
    int severity = 1; // 1-5
    int bedID = -1;

    
    int doctorAssigned = -1;
    int nurseAssigned = -1;

   
    bool equipmentAssigned = false;
};


class ICUBed
{
public:
    int id = 0;
    string status; 
};


class Equipment
{
public:
    int available = 0;
    int inUse = 0;
    int faulty = 0;
};


class Staff
{
public:
    int doctors = 0;
    int nurses = 0;
};


class FileManager
{
public:
    static void logAdmission(const Patient &p)
    {
        ofstream file("admissions.txt", ios::app);
        if (!file)
            return;

        file << nowTime()
             << " | Patient " << p.id
             << " admitted | Bed " << p.bedID
             << " | Sev " << p.severity
             << " | Dr " << p.doctorAssigned
             << " | Nurse " << p.nurseAssigned
             << " | Equip " << (p.equipmentAssigned ? "YES" : "NO")
             << " | Dx: " << p.diagnosis
             << "\n";
    }

    static void logDischarge(int id)
    {
        ofstream file("discharge.txt", ios::app);
        if (!file)
            return;

        file << nowTime() << " | Patient " << id << " discharged\n";
    }

    static void ensureReportHeader()
    {
        ifstream in("report.csv");
        bool empty = true;
        if (in)
            empty = (in.peek() == ifstream::traits_type::eof());
        in.close();

        if (empty)
        {
            ofstream out("report.csv", ios::app);
            if (!out)
                return;
            out << "Timestamp,TotalBeds,OccupiedBeds,AvailableEquip,InUseEquip,FaultyEquip,TotalPatients,BedOccupancyRate,EquipUtilRate\n";
        }
    }

    static void exportReportRow(const string &row)
    {
        ensureReportHeader();
        ofstream file("report.csv", ios::app);
        if (!file)
            return;
        file << row << "\n";
    }

    static void viewLogs(const string &filename)
    {
        ifstream file(filename);
        if (!file)
        {
            cout << "\nCould not open " << filename << " (file not found yet).\n";
            return;
        }

        string line;
        cout << "\n---- " << filename << " ----\n";
        while (getline(file, line))
            cout << line << "\n";
    }
};

class ReportGenerator
{
public:
    static void generate(int bedsTotal, int occupied,
                         const Equipment &eq, const Staff &staff,
                         int totalPatients)
    {
        cout << "\n---- ICU UTILIZATION REPORT ----\n";
        cout << "Total Beds: " << bedsTotal << "\n";
        cout << "Occupied Beds: " << occupied << "\n";
        cout << "Available Beds: " << (bedsTotal - occupied) << "\n";

        double bedRate = 0;
        if (bedsTotal > 0)
            bedRate = (double)occupied / bedsTotal * 100.0;
        cout << "Bed Occupancy Rate: " << bedRate << "%\n";

        int usableEquip = eq.available + eq.inUse; // exclude faulty
        double equipUtil = 0;
        if (usableEquip > 0)
            equipUtil = (double)eq.inUse / usableEquip * 100.0;

        cout << "\nEquipment\n";
        cout << "Available: " << eq.available << "\n";
        cout << "In Use: " << eq.inUse << "\n";
        cout << "Faulty: " << eq.faulty << "\n";
        cout << "Equipment Utilization (of usable): " << equipUtil << "%\n";

        cout << "\n--- Staff Workload ---\n";
        cout << "Doctors on duty: " << staff.doctors << "\n";
        cout << "Nurses on duty: " << staff.nurses << "\n";

        if (staff.nurses > 0)
            cout << "Patients per Nurse: " << (double)totalPatients / staff.nurses << "\n";
        if (staff.doctors > 0)
            cout << "Patients per Doctor: " << (double)totalPatients / staff.doctors << "\n";

        FileManager::exportReportRow(
            nowTime() + "," +
            to_string(bedsTotal) + "," +
            to_string(occupied) + "," +
            to_string(eq.available) + "," +
            to_string(eq.inUse) + "," +
            to_string(eq.faulty) + "," +
            to_string(totalPatients) + "," +
            to_string(bedRate) + "," +
            to_string(equipUtil));
    }
};


class ResourceManager
{
    vector<Patient> patients;
    vector<Patient> waitingList; 
    vector<ICUBed> beds;

    Equipment equipment;
    Staff staff;

    int nextDoctor = 1;
    int nextNurse = 1;

public:
    ResourceManager()
    {
        int capacity;
        cout << "Enter ICU Bed Capacity: ";
        cin >> capacity;

        for (int i = 1; i <= capacity; i++)
        {
            ICUBed b;
            b.id = i;
            b.status = "available";
            beds.push_back(b);
        }

        equipment.available = 3;
        equipment.inUse = 0;
        equipment.faulty = 0;

        staff.doctors = 2;
        staff.nurses = 4;
    }

    
    void showBeds()
    {
        cout << "\nICU BED STATUS\n";
        for (auto b : beds)
            cout << "Bed " << b.id << " : " << b.status << "\n";
    }


    void changeBedStatus()
    {
        int id;
        string status;

        cout << "Enter Bed ID: ";
        cin >> id;

        cout << "Enter status (available/occupied/reserved/maintenance): ";
        cin >> status;

        if (!(status == "available" || status == "occupied" || status == "reserved" || status == "maintenance"))
        {
            cout << "Invalid status.\n";
            return;
        }

        for (auto &b : beds)
        {
            if (b.id == id)
            {
                b.status = status;
                cout << "Status updated.\n";
                return;
            }
        }
        cout << "Bed not found.\n";
    }

   
    void addPatient()
    {
        Patient p;

        cout << "Enter Patient ID: ";
        cin >> p.id;
        clearLine();

        cout << "Enter Name: ";
        getline(cin, p.name);

        cout << "Enter Diagnosis: ";
        getline(cin, p.diagnosis);

        cout << "Enter Severity (1-5): ";
        cin >> p.severity;

        if (p.severity < 1)
            p.severity = 1;
        if (p.severity > 5)
            p.severity = 5;

        if (p.severity >= 4)
            cout << "High Priority Patient\n";

        assignBedOrWait(p);
    }

    
    void assignStaff(Patient &p)
    {
        if (staff.doctors > 0)
        {
            p.doctorAssigned = nextDoctor;
            nextDoctor++;
            if (nextDoctor > staff.doctors)
                nextDoctor = 1;
        }

        if (staff.nurses > 0)
        {
            p.nurseAssigned = nextNurse;
            nextNurse++;
            if (nextNurse > staff.nurses)
                nextNurse = 1;
        }

        cout << "Doctor assigned: " << (p.doctorAssigned == -1 ? 0 : p.doctorAssigned) << "\n";
        cout << "Nurse assigned: " << (p.nurseAssigned == -1 ? 0 : p.nurseAssigned) << "\n";
    }


  
    void assignBedOrWait(Patient p)
    {
       
        for (auto &b : beds)
        {
            if (b.status == "available")
            {
            
                p.bedID = b.id;

              
                b.status = "occupied";

                
                assignStaff(p);

              
                if (equipment.available > 0)
                {
                    equipment.available--;
                    equipment.inUse++;
                    p.equipmentAssigned = true;
                    cout << "Equipment assigned.\n";
                }
                else
                {
                    p.equipmentAssigned = false;
                    cout << "Warning: No equipment available right now.\n";
                }

               
                patients.push_back(p);

                cout << "Bed " << b.id << " assigned.\n";

         
                FileManager::logAdmission(p);
                return;
            }
        }

      
        waitingList.push_back(p);
        cout << "No ICU Beds Available. Patient added to waiting list.\n";
        cout << "Waiting List Size: " << waitingList.size() << "\n";
    }


    void allocateFromWaiting()
    {
        if (waitingList.empty())
            return;

        
        int bestIdx = 0;
        for (int i = 1; i < (int)waitingList.size(); i++)
        {
            if (waitingList[i].severity > waitingList[bestIdx].severity)
                bestIdx = i;
        }

        Patient p = waitingList[bestIdx];
        waitingList.erase(waitingList.begin() + bestIdx);

        cout << "\nAllocating bed to waiting patient ID " << p.id
             << " (Severity " << p.severity << ")\n";

        assignBedOrWait(p);
    }

   
    void discharge()
    {
        int id;
        cout << "Enter Patient ID: ";
        cin >> id;

        for (int i = 0; i < (int)patients.size(); i++)
        {
            if (patients[i].id == id)
            {
                int bed = patients[i].bedID;

              
                for (auto &b : beds)
                {
                    if (b.id == bed)
                    {
                        
                        b.status = "available";
                        break;
                    }
                }

                if (patients[i].equipmentAssigned)
                {
                    if (equipment.inUse > 0)
                        equipment.inUse--;
                    equipment.available++;
                }

                patients.erase(patients.begin() + i);

                FileManager::logDischarge(id);

                cout << "Patient discharged.\n";

               
                allocateFromWaiting();
                return;
            }
        }
        cout << "Patient not found.\n";
    }

   
    void showEquipment()
    {
        cout << "\nEquipment Status\n";
        cout << "Available: " << equipment.available << "\n";
        cout << "In Use: " << equipment.inUse << "\n";
        cout << "Faulty: " << equipment.faulty << "\n";

        if (equipment.available < 2)
            cout << "Alert: Equipment below minimum threshold!\n";
    }

   
    void markFaulty()
    {
        cout << "\n1. Mark AVAILABLE equipment faulty\n";
        cout << "2. Mark IN-USE equipment faulty (urgent)\n";
        cout << "Enter choice: ";
        int c;
        cin >> c;

        if (c == 1)
        {
            if (equipment.available > 0)
            {
                equipment.available--;
                equipment.faulty++;
                cout << "Equipment marked faulty.\n";
            }
            else
            {
                cout << "No available equipment to mark faulty.\n";
            }
        }
        else if (c == 2)
        {
            if (equipment.inUse > 0)
            {
                equipment.inUse--;
                equipment.faulty++;
                cout << "In-use equipment marked faulty.\n";
            }
            else
            {
                cout << "No in-use equipment to mark faulty.\n";
            }
        }
        else
        {
            cout << "Invalid.\n";
        }
    }

    /* Repair Faulty Equipment */
    void repairFaulty()
    {
        if (equipment.faulty > 0)
        {
            equipment.faulty--;
            equipment.available++;
            cout << "One faulty equipment repaired and moved to available.\n";
        }
        else
        {
            cout << "No faulty equipment to repair.\n";
        }
    }

  
    void showStaff()
    {
        cout << "\nStaff Status\n";
        cout << "Doctors: " << staff.doctors << "\n";
        cout << "Nurses: " << staff.nurses << "\n";

        if ((int)patients.size() > staff.nurses)
            cout << "Warning: Staff overload!\n";
    }

    
    void showWaitingList()
    {
        cout << "\n---- WAITING LIST ----\n";
        if (waitingList.empty())
        {
            cout << "No patients in waiting list.\n";
            return;
        }

        for (auto &p : waitingList)
            cout << "ID: " << p.id
                 << " | Name: " << p.name
                 << " | Sev: " << p.severity
                 << " | Dx: " << p.diagnosis << "\n";
    }

   
    void report()
    {
        int occupied = 0;
        for (auto b : beds)
            if (b.status == "occupied")
                occupied++;

        ReportGenerator::generate((int)beds.size(), occupied, equipment, staff, (int)patients.size());

        double rate = 0;
        if (!beds.empty())
            rate = (double)occupied / beds.size() * 100.0;

        if (rate >= 80)
            cout << "ALERT: ICU occupancy above threshold (80%)\n";

        if (occupied == (int)beds.size() && !beds.empty())
            cout << "ALERT: ICU FULL\n";
    }

    
    void viewLogs()
    {
        cout << "\n1. Admission Logs\n";
        cout << "2. Discharge Logs\n";
        cout << "Enter choice: ";
        int c;
        cin >> c;

        if (c == 1)
            FileManager::viewLogs("admissions.txt");
        else if (c == 2)
            FileManager::viewLogs("discharge.txt");
        else
            cout << "Invalid.\n";
    }
};


int main()
{
    ResourceManager icu;
    int choice;

    while (true)
    {
        cout << "\n===== ICU RESOURCE SYSTEM =====\n";
        cout << "1. Show Beds\n";
        cout << "2. Register Patient\n";
        cout << "3. Discharge Patient\n";
        cout << "4. Equipment Status\n";
        cout << "5. Mark Equipment Faulty\n";
        cout << "6. Repair Faulty Equipment\n";
        cout << "7. Staff Status\n";
        cout << "8. Generate Report\n";
        cout << "9. Change Bed Status\n";
        cout << "10. View Logs\n";
        cout << "11. Show Waiting List\n";
        cout << "12. Exit\n";

        cout << "Enter Choice: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            icu.showBeds();
            break;
        case 2:
            icu.addPatient();
            break;
        case 3:
            icu.discharge();
            break;
        case 4:
            icu.showEquipment();
            break;
        case 5:
            icu.markFaulty();
            break;
        case 6:
            icu.repairFaulty();
            break;
        case 7:
            icu.showStaff();
            break;
        case 8:
            icu.report();
            break;
        case 9:
            icu.changeBedStatus();
            break;
        case 10:
            icu.viewLogs();
            break;
        case 11:
            icu.showWaitingList();
            break;
        case 12:
            return 0;
        default:
            cout << "Invalid choice\n";
        }
    }
}