//
//  ASCOM_DDE.h
//  nebulosity3
//
//  Created by Craig Stark on 4/7/14.
//  Copyright (c) 2014 Stark Labs. All rights reserved.
//

#include "wx/ipc.h"

#ifndef DDECLASSES
#define DDECLASSES
class DDEConnection : public wxConnection
{
public:
    virtual bool OnExecute(const wxString& topic, const void *data, size_t size, wxIPCFormat format);
    virtual const void *OnRequest(const wxString& topic, const wxString& item, size_t *size, wxIPCFormat format);
    virtual bool OnPoke(const wxString& topic, const wxString& item, const void *data, size_t size, wxIPCFormat format);
    virtual bool OnStartAdvise(const wxString& topic, const wxString& item);
    virtual bool OnStopAdvise(const wxString& topic, const wxString& item);
    virtual bool DoAdvise(const wxString& item, const void *data, size_t size, wxIPCFormat format);
    virtual bool OnDisconnect();
	long *imgdata;

    // topic for which we advise the client or empty if none
    wxString m_advise;

protected:
    // the data returned by last OnRequest(): we keep it in this buffer to
    // ensure that the pointer we return from OnRequest() stays valid
    wxCharBuffer m_requestData;

};

class DDEServer : public wxServer
{
public:
    DDEServer();
    virtual ~DDEServer();

    void Disconnect();
    bool IsConnected() { return m_connection != NULL; }
    DDEConnection *GetConnection() { return m_connection; }

    void Advise();
    bool CanAdvise() { return m_connection && !m_connection->m_advise.empty(); }

    virtual wxConnectionBase *OnAcceptConnection(const wxString& topic);

protected:
    DDEConnection *m_connection;
};

#endif // DDECLASSES