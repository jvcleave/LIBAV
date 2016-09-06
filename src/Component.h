#pragma once
#include "OMX_Maps.h"



class ComponentListener
{
public:
    virtual void onFillBuffer()=0;
    virtual void onEmptyBuffer()=0;
    virtual void onEvent(OMX_EVENTTYPE eEvent)=0;
};

struct omx_event 
{
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
};



class Component
{
public:
    
    OMX_HANDLETYPE handle;
    OMX_CALLBACKTYPE  callbacks;
    string name;
    std::vector<omx_event> m_omx_events;
    
    
    //OMX_ERRORTYPE (*FillBufferDoneHandler)(OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE*);
    //OMX_ERRORTYPE (*EmptyBufferDoneHandler)();
   // OMX_ERRORTYPE (*EventHandler)(OMX_HANDLETYPE, OMX_PTR, OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR);
    
    vector<OMX_BUFFERHEADERTYPE*>   inputBuffers;
    queue<OMX_BUFFERHEADERTYPE*>    inputBuffersAvailable;
    condition_variable      m_input_buffer_cond;
    std::mutex          m_omx_input_mutex;
    std::mutex          m_omx_event_mutex;
    static void add_timespecs(struct timespec& time, long millisecs)
    {
        time.tv_sec  += millisecs / 1000;
        time.tv_nsec += (millisecs % 1000) * 1000000;
        if (time.tv_nsec > 1000000000)
        {
            time.tv_sec  += 1;
            time.tv_nsec -= 1000000000;
        }
    }
    ComponentListener* listener;
    
    static 
    OMX_ERRORTYPE onEmptyBufferDone(OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_BUFFERHEADERTYPE* pBuffer)
    {
        
        
        
       Component *component = (Component*)pAppData;
        
        component->m_omx_input_mutex.lock();

        component->inputBuffersAvailable.push(pBuffer);
        
        component->m_omx_input_mutex.unlock();
        
        
        
        if(component->listener)
        {
            component->listener->onEmptyBuffer();
        }
        return OMX_ErrorNone;
    }
    
    static 
    OMX_ERRORTYPE onFillBufferDone(OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_BUFFERHEADERTYPE* pBuffer)
    {
        Component *component = (Component*)pAppData;
        if(component->listener)
        {
            component->listener->onFillBuffer();
        }
        
        return OMX_ErrorNone;
    }
    
    static 
    OMX_ERRORTYPE onEvent(OMX_HANDLETYPE hComponent,
                          OMX_PTR pAppData,
                          OMX_EVENTTYPE eEvent,
                          OMX_U32 nData1,
                          OMX_U32 nData2,
                          OMX_PTR pEventData)
    {
        OMX_ERRORTYPE error = OMX_ErrorNone;

        Component *component = (Component *)pAppData;
        if(component->listener)
        {
            component->listener->onEvent(eEvent);
            return error;
        }
        stringstream info;
        info << "name: " << component->name << endl;
        info << "EVENT TYPE: " << OMX_Maps::getInstance().eventTypes[eEvent] << endl;
        if(eEvent == OMX_EventError)
        {
            info << "ERROR TYPE: " << omxErrorToString((OMX_ERRORTYPE)nData1) << endl;
            
            
            OMX_STATETYPE state;
            error = OMX_GetState(hComponent, &state);
            OMX_TRACE(error);
            info << "state: " << OMX_Maps::getInstance().omxStateNames[state];
            
        }
        
        ofLogVerbose(__func__) << info.str();
  
        
        switch (eEvent) 
        {
            case OMX_EventCmdComplete:
            {
                break;
            }
            case OMX_EventPortSettingsChanged:
            {
                //component->onPortSettingsChanged();
                break;
            }
            case OMX_EventParamOrConfigChanged:
            {
                
                //return component->onCameraEventParamOrConfigChanged();
                break;
            }	
                
            case OMX_EventError:
            {
                error = (OMX_ERRORTYPE)nData1;
                break;
                
            }
            default: 
            {
                /* ofLog(OF_LOG_VERBOSE, 
                 "onEncoderEvent::%s - eEvent(0x%x), nData1(0x%ux), nData2(0x%ux), pEventData(0x%p)\n",
                 __func__, eEvent, nData1, nData2, pEventData);*/
                
                break;
            }
        }
        OMX_TRACE(error);
        return error;

        
        return OMX_ErrorNone;
    }
    

    
    Component()
    {
        name = OMX_NULL_SINK;
        //FillBufferDoneHandler = NULL;
        //EmptyBufferDoneHandler = NULL;
        listener = NULL;
    }
    
    OMX_HANDLETYPE setup(ComponentListener* listener_=NULL)
    {
        if(listener_)
        {
           listener = listener_; 
        }
        
        //pthread_mutex_init(&m_omx_input_mutex, NULL);
        //pthread_cond_init(&m_input_buffer_cond, NULL);
        
        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        
        callbacks.EventHandler    = &Component::onEvent;
        callbacks.EmptyBufferDone = &Component::onEmptyBufferDone;
        callbacks.FillBufferDone  = &Component::onFillBufferDone;
        ofLogVerbose(__func__) << "name: " << name;
        
        error = OMX_GetHandle(&handle, (OMX_STRING)name.c_str(), this, &callbacks);
        OMX_TRACE(error, "OMX_GetHandle");
        if(handle)
        {
            error = disableAllPortsForComponent(handle);
            OMX_TRACE(error);
        }
 
        return handle;
    }
    
    OMX_BUFFERHEADERTYPE* getInputBuffer(long timeout)
    {
        if(!handle) 
        {
            ofLogError(__func__) << name << "has NO handle";
        }
        
        OMX_BUFFERHEADERTYPE* inputBuffer = NULL;
        
        
        m_omx_input_mutex.lock();
        struct timespec endtime;
        clock_gettime(CLOCK_REALTIME, &endtime);
        add_timespecs(endtime, timeout);
        while (1)
        {
            if(!inputBuffersAvailable.empty())
            {
                inputBuffer = inputBuffersAvailable.front();
                inputBuffersAvailable.pop();
                break;
            }
            /*
            int retcode = pthread_cond_timedwait(&m_input_buffer_cond, &m_omx_input_mutex, &endtime);
            if (retcode != 0)
            {
                ofLogError(__func__) << "DECODER TIMEOUT AT: " << timeout;
                break;
            }*/
        }
        m_omx_input_mutex.unlock();
        return inputBuffer;
    }
    
    OMX_ERRORTYPE 
    DisablePort(unsigned int port, bool wait=true)
    {

        
        OMX_ERRORTYPE error = OMX_ErrorNone;
        
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = port;
        
        error = OMX_GetParameter(handle, OMX_IndexParamPortDefinition, &portFormat);
        OMX_TRACE(error);
        
        if(portFormat.bEnabled == OMX_TRUE)
        {
            error = OMX_SendCommand(handle, OMX_CommandPortDisable, port, NULL);
            OMX_TRACE(error);
            if(error == OMX_ErrorNone)
            {
                if(wait)
                {
                    error = WaitForCommand(OMX_CommandPortDisable, port);
                }
            }
            else
            {
                
            }
        }
        return error;
    }
    
#define OMX_DEBUG_EVENTS 1
    
    // timeout in milliseconds
    OMX_ERRORTYPE WaitForCommand(OMX_U32 command, OMX_U32 nData2, long timeout=2000)
    {
#ifdef OMX_DEBUG_EVENTS
        ofLog(OF_LOG_VERBOSE, "COMXCoreComponent::WaitForCommand %s wait event.eEvent 0x%08x event.command 0x%08x event.nData2 %d\n", 
                  name.c_str(), (int)OMX_EventCmdComplete, (int)command, (int)nData2);
#endif
        
        m_omx_event_mutex.lock();
        struct timespec endtime;
        clock_gettime(CLOCK_REALTIME, &endtime);
        add_timespecs(endtime, timeout);
        while(true) 
        {
            for (std::vector<omx_event>::iterator it = m_omx_events.begin(); it != m_omx_events.end(); it++) 
            {
                omx_event event = *it;
                
#ifdef OMX_DEBUG_EVENTS
                ofLog(OF_LOG_VERBOSE, "COMXCoreComponent::WaitForCommand %s inlist event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d\n",
                          name.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
#endif
                if(event.eEvent == OMX_EventError && event.nData1 == (OMX_U32)OMX_ErrorSameState && event.nData2 == 1)
                {
#ifdef OMX_DEBUG_EVENTS
                    ofLog(OF_LOG_VERBOSE, "COMXCoreComponent::WaitForCommand %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d\n",
                              name.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
#endif
                    
                    m_omx_events.erase(it);
                    m_omx_event_mutex.unlock();
                    return OMX_ErrorNone;
                } 
                else if(event.eEvent == OMX_EventError) 
                {
                    m_omx_events.erase(it);
                    m_omx_event_mutex.unlock();
                    return (OMX_ERRORTYPE)event.nData1;
                } 
                else if(event.eEvent == OMX_EventCmdComplete && event.nData1 == command && event.nData2 == nData2) 
                {
                    
#ifdef OMX_DEBUG_EVENTS
                    ofLog(OF_LOG_VERBOSE, "COMXCoreComponent::WaitForCommand %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d\n",
                              name.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
#endif
                    
                    m_omx_events.erase(it);
                    m_omx_event_mutex.unlock();
                    return OMX_ErrorNone;
                }
            }
            /*
            if (m_resource_error)
            {
                 break;
            }
            int retcode = pthread_cond_timedwait(&m_omx_event_cond, &m_omx_event_mutex, &endtime);
            if (retcode != 0) {
                ofLog(OF_LOG_ERROR, "COMXCoreComponent::WaitForCommand %s wait timeout event.eEvent 0x%08x event.command 0x%08x event.nData2 %d\n", 
                          name.c_str(), (int)OMX_EventCmdComplete, (int)command, (int)nData2);
                
                m_omx_event_mutex.unlock();
                return OMX_ErrorTimeout;
            }*/
        }
        m_omx_event_mutex.unlock();
        return OMX_ErrorNone;
    }
    
    
    

    
    
    OMX_ERRORTYPE AddEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
    {
        omx_event event;
        
        event.eEvent      = eEvent;
        event.nData1      = nData1;
        event.nData2      = nData2;
        
        m_omx_event_mutex.lock();
        for (std::vector<omx_event>::iterator it = m_omx_events.begin(); it != m_omx_events.end(); ) 
        {
            omx_event event = *it;
            
            if(event.eEvent == eEvent && event.nData1 == nData1 && event.nData2 == nData2) 
            {
                it = m_omx_events.erase(it);
                continue;
            }
            ++it;
        }
        m_omx_events.push_back(event);
        // this allows (all) blocked tasks to be awoken
        //pthread_cond_broadcast(&m_omx_event_cond);
        m_omx_event_mutex.lock();
        
#ifdef OMX_DEBUG_EVENTS
        ofLog(OF_LOG_VERBOSE, "COMXCoreComponent::AddEvent %s add event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d\n",
                  name.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
#endif
        
        return OMX_ErrorNone;
    }

    
    

    
    
    
};