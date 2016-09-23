#pragma once
#include "OMX_Maps.h"



class ComponentListener
{
public:
    virtual void onFillBuffer()=0;
    virtual void onEmptyBuffer()=0;
    virtual void onEvent(OMX_EVENTTYPE eEvent)=0;
};



class Component
{
public:
    
    OMX_HANDLETYPE handle;
    OMX_CALLBACKTYPE  callbacks;
    string name;
    vector<OMX_BUFFERHEADERTYPE*>   inputBuffers;
    queue<OMX_BUFFERHEADERTYPE*>    inputBuffersAvailable;
    condition_variable      m_input_buffer_cond;
    std::mutex          m_omx_input_mutex;

    ComponentListener* listener;
    
    static 
    OMX_ERRORTYPE onEmptyBufferDone(OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_BUFFERHEADERTYPE* pBuffer)
    {
        
        
        START;
        Component *component = (Component*)pAppData;
        if(component->m_omx_input_mutex.try_lock())
        {
            ofLogVerbose(__func__) << "lock";
            component->inputBuffersAvailable.push(pBuffer);
            ofLogVerbose() << "component->inputBuffersAvailable: " << component->inputBuffersAvailable.size();
            component->m_omx_input_mutex.unlock();
            ofLogVerbose(__func__) << "unlock";
            
            if(component->listener)
            {
                component->listener->onEmptyBuffer();
            }
        }else
        {
            ofLogError(__func__) << "ALREADY LOCKED";
        }
        
        END; 
        
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
    

    OMX_BUFFERHEADERTYPE* getInputBuffer()
    {
        if(!handle) 
        {
            ofLogError(__func__) << name << "has NO handle";
        }
        
        OMX_BUFFERHEADERTYPE* inputBuffer = NULL;
        
        std::unique_lock<mutex> lock(m_omx_input_mutex);
        if(!inputBuffersAvailable.empty())
        {
            inputBuffer = inputBuffersAvailable.front();
            inputBuffersAvailable.pop();
        }
        lock.unlock();
        return inputBuffer;
    }
    

    
    

    
    
    
};