#ifndef TASKMEMENTO_H
#define TASKMEMENTO_H

#include <string>
#include <map>
#include "Support.h" // this was the crazy error sigh 

class Task;

/**
 * @brief Stores a snapshot of a Task's state.
 * 
 * So the state of a task can be restored later
*/


class TaskMemento {
    private:
        /**
         * @brief Task attributes that are stored in the memento
        */
        std::string name;
        std::string description;
        std::string executorId;
        DateTime deadline;
        std::vector<Participant> participants;       
        std::map<std::string, std::string> customFields;
        

        /**
         * @brief Creates a memento from the given Task
         * @param The task whose state will be stored
         */
        TaskMemento(const Task& task);

    public:
        /**
         * @brief Creates an empty TaskMemento
        */
        TaskMemento();
        
        // helper
        /**
         * @brief Checks whether the memento is empty.
         * @return True if the memento contains no state.
         */
        bool isEmpty() const;
    
    /**
     * @brief Allows Task to access the private members of TaskMemento
     * 
     * Enables Taskto save and restore its state using the memento
     */
    friend class Task; 
};

#endif