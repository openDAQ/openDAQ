#include <opendaq/packet.h>
#include <opendaq/reader_factory.h>
#include <iostream>
#include "idds/idds_server.h"

BEGIN_NAMESPACE_OPENDAQ_IDDS

//--------------------------------------------------------------------------------------------------
// Constants.
//--------------------------------------------------------------------------------------------------
static const int hello_interval = 5; // seconds
static const char node_advertiser_topic[] = "idds_hello";

// Constructor
iDDSServer::iDDSServer(const iDDSNodeUniqueID node_id) : node_id(node_id),
                                                         listenerNodeAdvertisement_impl(new DataReaderListenerImpl),
                                                         listenerNodeAdvertisement(listenerNodeAdvertisement_impl),
                                                         listenerCommand_impl(new CommandListenerImpl),
                                                         listenerCommand(listenerCommand_impl)
{
    SetupiDDSServer();
}

// Destructor
iDDSServer::~iDDSServer()
{
    // Join threads if they are joinable
    if (m_bRunning)
    {
        m_bRunning = false;

        if (advertiser_thread.joinable())
        {
            advertiser_thread.join();
        }

        if (listener_thread.joinable())
        {
            listener_thread.join();
        }

        if (command_listener_thread.joinable())
        {
            command_listener_thread.join();
        }
    }

    // Clean up
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
}

/// Initialize Domain Factory and Participant
int iDDSServer::SetupiDDSServer()
{
    // Initialize DomainParticipantFactory
    dpf = TheServiceParticipant->get_domain_participant_factory();

    // Set up the default info repo
    const char *repo_ior = "file://simple.ior";
    OpenDDS::DCPS::Service_Participant *service_participant = TheServiceParticipant;
    service_participant->set_repo_ior(repo_ior, OpenDDS::DCPS::Discovery::DEFAULT_REPO);

    // Create DomainParticipant
    participant =
        dpf->create_participant(42,
                                PARTICIPANT_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_participant failed!\n")),
                         1);
    }

    return 0;
}

// StartServer method
void iDDSServer::StartServer()
{
    // Start the server
    m_bRunning = true;

    advertiser_thread = std::thread(&iDDSServer::NodeAdvertiser, this);
    listener_thread = std::thread(&iDDSServer::ListenForNodeAdvertisementMessages, this);
    command_listener_thread = std::thread(&iDDSServer::ListenForCommandMessages, this);
}

// GetAvailableiDDSServers method
std::vector<iDDSServer::iDDSNodeUniqueID> iDDSServer::GetAvailableiDDSServers()
{
    std::vector<Messenger::iDDSHelloMsg> vec = listenerNodeAdvertisement_impl->get_message_vector();
    std::vector<iDDSNodeUniqueID> available_devices;

    for (const auto &msg : vec)
    {
        available_devices.push_back(msg.unique_id.in());
    }

    // Placeholder implementation
    return available_devices;
}

// SendIDDSMessage method
int iDDSServer::SendIDDSMessage(const iDDSNodeUniqueID destination_node_id, const std::string message_)
{
    try
    {
        // Register TypeSupport (Messenger::iDDSControlMsg)
        Messenger::iDDSControlMsgTypeSupport_var ts =
            new Messenger::iDDSControlMsgTypeSupportImpl;

        if (ts->register_type(participant, "") != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" register_type failed!\n")),
                             1);
        }

        // Create Topic
        CORBA::String_var type_name = ts->get_type_name();
        DDS::Topic_var topic =
            participant->create_topic("idds_control",
                                      type_name,
                                      TOPIC_QOS_DEFAULT,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!topic)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_topic failed!\n")),
                             1);
        }

        // Create Publisher
        DDS::Publisher_var publisher =
            participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                          0,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!publisher)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_publisher failed!\n")),
                             1);
        }

        // Create DataWriter
        DDS::DataWriter_var writer =
            publisher->create_datawriter(topic,
                                         DATAWRITER_QOS_DEFAULT,
                                         0,
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!writer)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_datawriter failed!\n")),
                             1);
        }

        Messenger::iDDSControlMsgDataWriter_var iDDSControlMsg_writer =
            Messenger::iDDSControlMsgDataWriter::_narrow(writer);

        if (!iDDSControlMsg_writer)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" _narrow failed!\n")),
                             1);
        }

        // Block until Subscriber is available
        DDS::StatusCondition_var condition = writer->get_statuscondition();
        condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        //ACE_DEBUG((LM_DEBUG,
        //           ACE_TEXT("Block until subscriber is available\n")));

        while (true)
        {
            DDS::PublicationMatchedStatus matches;
            if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
            {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -")
                                      ACE_TEXT(" get_publication_matched_status failed!\n")),
                                 1);
            }

            if (matches.current_count >= 1)
            {
                break;
            }

            DDS::ConditionSeq conditions;
            DDS::Duration_t timeout = {60, 0};
            if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
            {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -")
                                      ACE_TEXT(" wait failed!\n")),
                                 1);
            }
        }

        //ACE_DEBUG((LM_DEBUG,
        //           ACE_TEXT("Subscriber is available\n")));

        ws->detach_condition(condition);

        // Write samples
        Messenger::iDDSControlMsg message;
        message.timestamp = 0;
        message.tid = 0;
        message.from = node_id.c_str();
        message.to = destination_node_id.c_str();

        // The string you want to insert into the command sequence
        const char *command_string = message_.c_str();

        // Determine the length of the string
        size_t string_length = std::strlen(command_string);

        // Make sure the string length does not exceed the max length
        if (string_length > Messenger::IDDS_COMMAND_MAX_LENGTH)
        {
            // Handle the error: string is too long to fit into the iDDSCommand
            std::cerr << "Error: Command string is too long!" << std::endl;
            return 1;
        }

        // Create and set the length of the iDDSCommand sequence
        Messenger::iDDSCommand command;
        command.length(static_cast<CORBA::ULong>(string_length));

        // Copy the string into the command sequence
        std::memcpy(command.get_buffer(), command_string, string_length);

        // Assign the sequence to the command field of the control message
        message.command = command;

        DDS::ReturnCode_t error = iDDSControlMsg_writer->write(message, DDS::HANDLE_NIL);

        if (error != DDS::RETCODE_OK)
        {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: %N:%l: main() -")
                           ACE_TEXT(" write returned %d!\n"),
                       error));
        }

        // Wait for samples to be acknowledged
        DDS::Duration_t timeout = {30, 0};
        if (iDDSControlMsg_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                             1);
        }

        return 0;
    }
    catch (const CORBA::Exception &e)
    {
        e._tao_print_exception("Exception caught in main():");
        return 1;
    }
}

// PrintReceivedIDDSMessages method
void iDDSServer::PrintReceivedIDDSMessages()
{
    // Placeholder implementation
    int i = 0;

    std::vector<Messenger::iDDSControlMsg> vec = listenerCommand_impl->get_message_vector();

    for (Messenger::iDDSControlMsg msg : vec)
    {
        std::cout << "#" << i++ << " - ";
        std::cout << "from: = " << msg.from;
        std::cout << "; to: = " << msg.to << std::endl;

        // Printing the command sequence
        if (msg.command.length() > 0) {
            std::string command_str(reinterpret_cast<const char*>(msg.command.get_buffer()), msg.command.length());
            std::cout << "message: = " << command_str << std::endl;
        } else {
            std::cout << "message: = [empty]" << std::endl;
        }
    }
}

// NodeAdvertiser method
void iDDSServer::NodeAdvertiser()
{
    // Placeholder implementation
    std::cout << "Node advertiser started." << std::endl;

    while (m_bRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(hello_interval)); // Wait for 5 seconds
        if (m_bRunning)
        {
            SendAdvertisementMessage();
        }
    }
}

// ListenForNodeAdvertisementMessages method
int iDDSServer::ListenForNodeAdvertisementMessages()
{
    // Register TypeSupport (Messenger::iDDSHelloMsg)
    Messenger::iDDSHelloMsgTypeSupport_var ts =
        new Messenger::iDDSHelloMsgTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" register_type failed!\n")),
                         1);
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant->create_topic("idds_hello",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_topic failed!\n")),
                         1);
    }

    // Create Subscriber
    DDS::Subscriber_var subscriber =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_subscriber failed!\n")),
                         1);
    }

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
        subscriber->create_datareader(topic,
                                      reader_qos,
                                      listenerNodeAdvertisement,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_datareader failed!\n")),
                         1);
    }

    Messenger::iDDSHelloMsgDataReader_var reader_i =
        Messenger::iDDSHelloMsgDataReader::_narrow(reader);

    if (!reader_i)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" _narrow failed!\n")),
                         1);
    }

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true)
    {
        DDS::SubscriptionMatchedStatus matches;
        if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" get_subscription_matched_status failed!\n")),
                             1);
        }

        if (matches.current_count == 0 && matches.total_count > 0)
        {
            break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = {60, 0};
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" wait failed!\n")),
                             1);
        }
    }

    ws->detach_condition(condition);

    return 0;
}

int iDDSServer::ListenForCommandMessages()
{
    // Register TypeSupport (Messenger::iDDSControlMsg)
    Messenger::iDDSControlMsgTypeSupport_var ts =
        new Messenger::iDDSControlMsgTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" register_type failed!\n")),
                         1);
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant->create_topic("idds_control",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_topic failed!\n")),
                         1);
    }

    // Create Subscriber
    DDS::Subscriber_var subscriber =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_subscriber failed!\n")),
                         1);
    }

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
        subscriber->create_datareader(topic,
                                      reader_qos,
                                      listenerCommand,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" create_datareader failed!\n")),
                         1);
    }

    Messenger::iDDSControlMsgDataReader_var reader_i =
        Messenger::iDDSControlMsgDataReader::_narrow(reader);

    if (!reader_i)
    {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" _narrow failed!\n")),
                         1);
    }

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true)
    {
        DDS::SubscriptionMatchedStatus matches;
        if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" get_subscription_matched_status failed!\n")),
                             1);
        }

        if (matches.current_count == 0 && matches.total_count > 0)
        {
            break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = {60, 0};
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" wait failed!\n")),
                             1);
        }
    }

    ws->detach_condition(condition);

    return 0;
}

int iDDSServer::SendAdvertisementMessage()
{
    try
    {
        // Register TypeSupport (Messenger::iDDSHelloMsg)
        Messenger::iDDSHelloMsgTypeSupport_var ts =
            new Messenger::iDDSHelloMsgTypeSupportImpl;

        if (ts->register_type(participant, "") != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" register_type failed!\n")),
                             1);
        }

        // Create Topic
        CORBA::String_var type_name = ts->get_type_name();
        DDS::Topic_var topic =
            participant->create_topic("idds_hello",
                                      type_name,
                                      TOPIC_QOS_DEFAULT,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!topic)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_topic failed!\n")),
                             1);
        }

        // Create Publisher
        DDS::Publisher_var publisher =
            participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                          0,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!publisher)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_publisher failed!\n")),
                             1);
        }

        // Create DataWriter
        DDS::DataWriter_var writer =
            publisher->create_datawriter(topic,
                                         DATAWRITER_QOS_DEFAULT,
                                         0,
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!writer)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" create_datawriter failed!\n")),
                             1);
        }

        Messenger::iDDSHelloMsgDataWriter_var iDDSHelloMsg_writer =
            Messenger::iDDSHelloMsgDataWriter::_narrow(writer);

        if (!iDDSHelloMsg_writer)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" _narrow failed!\n")),
                             1);
        }

        // Block until Subscriber is available
        DDS::StatusCondition_var condition = writer->get_statuscondition();
        condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        //ACE_DEBUG((LM_DEBUG,
        //           ACE_TEXT("Block until subscriber is available\n")));

        while (true)
        {
            DDS::PublicationMatchedStatus matches;
            if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
            {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -")
                                      ACE_TEXT(" get_publication_matched_status failed!\n")),
                                 1);
            }

            if (matches.current_count >= 1)
            {
                break;
            }

            DDS::ConditionSeq conditions;
            DDS::Duration_t timeout = {60, 0};
            if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
            {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("ERROR: %N:%l: main() -")
                                      ACE_TEXT(" wait failed!\n")),
                                 1);
            }
        }

        //ACE_DEBUG((LM_DEBUG,
        //           ACE_TEXT("Subscriber is available\n")));

        ws->detach_condition(condition);

        // Write samples
        Messenger::iDDSHelloMsg message;
        message.timestamp = 0;
        message.unique_id = node_id.c_str();
        message.manufacturer = "OpenDAQ";
        message.model = "model";
        message.serial_number = "serial_number";
        message.logical_id = node_id.c_str();
        message.tags = "tags";

        DDS::ReturnCode_t error = iDDSHelloMsg_writer->write(message, DDS::HANDLE_NIL);

        if (error != DDS::RETCODE_OK)
        {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: %N:%l: main() -")
                           ACE_TEXT(" write returned %d!\n"),
                       error));
        }

        // Wait for samples to be acknowledged
        DDS::Duration_t timeout = {30, 0};
        if (iDDSHelloMsg_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK)
        {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: main() -")
                                  ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                             1);
        }
    }
    catch (const CORBA::Exception &e)
    {
        e._tao_print_exception("Exception caught in main():");
        return 1;
    }

    return 0;
}

END_NAMESPACE_OPENDAQ_IDDS
