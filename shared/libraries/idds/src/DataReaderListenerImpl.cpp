/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "DataReaderListenerImpl.h"

#include <iostream>

void DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedDeadlineMissedStatus & /*status*/)
{
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedIncompatibleQosStatus & /*status*/)
{
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SampleRejectedStatus & /*status*/)
{
}

void DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::LivelinessChangedStatus & /*status*/)
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  Messenger::iDDSHelloMsgDataReader_var reader_i =
      Messenger::iDDSHelloMsgDataReader::_narrow(reader);

  if (!reader_i)
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                   ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(1);
  }

  Messenger::iDDSHelloMsg message;
  DDS::SampleInfo info;

  const DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK)
  {
    if (info.valid_data)
    {
      // Debugging version of unique_id comparison
      auto it = std::find_if(message_vector.begin(), message_vector.end(),
                             [&message](const Messenger::iDDSHelloMsg &msg)
                             {
                              return std::strcmp(msg.unique_id.in(), message.unique_id.in()) == 0;
                             });

      // If unique_id is not found, append the new message to the vector
      if (it == message_vector.end())
      {
        message_vector.push_back(message);
      }
    }
  }
  else
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                   ACE_TEXT(" take_next_sample failed!\n")));
  }
}

void DataReaderListenerImpl::on_subscription_matched(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SubscriptionMatchedStatus & /*status*/)
{
}

void DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SampleLostStatus & /*status*/)
{
}

std::vector<Messenger::iDDSHelloMsg> DataReaderListenerImpl::get_message_vector()
{
  return message_vector;
}