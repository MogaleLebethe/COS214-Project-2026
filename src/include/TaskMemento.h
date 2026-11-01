#ifndef TASKMEMENTO_H
#define TASKMEMENTO_H

#include <string>
#include <map>

class Task;
class DateTime; 

class TaskMemento {
    private:
        std::string name;
        std::string description;
        std::string executorId;
        DateTime deadline;
        std::map<std::string, std::string> customFields;
        
        TaskMemento(const Task& task);

    public:
        TaskMemento();
        
        // helper
        bool isEmpty() const;
    
        
    friend class Task; 
};



#endif