using System.Collections.Generic;
using System.Text;
using RTGen.Interfaces;

namespace RTGen.Cpp.Generators
{
    static class Event
    {
        public static string Fields(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 40;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            sb.AppendLine($"    template <typename TEventArgs>");
            sb.AppendLine($"    using Event = Dewesoft::MUI::Event<{rtClass.Type.Wrapper.Name}, TEventArgs>;");
            sb.AppendLine();

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;
                string eventArgsTypePtr = rtEvent.Value.EventArgsType == null
                                              ? "Dewesoft::MUI::EventArgs"
                                              : rtEvent.Value.EventArgsType.Wrapper.NameFull;

                sb.AppendLine($"    Event<{eventArgsTypePtr}> {eventName};");
            }

            return sb.ToString();
        }

        public static string CopyWrappers(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 30;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;

                sb.AppendLine($"        , {eventName}(other.{eventName}, this)");
            }

            return sb.ToString();
        }

        public static string MoveWrappers(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 40;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;

                sb.AppendLine($"        , {eventName}(std::move(other.{eventName}), this)");
            }

            return sb.ToString();
        }

        public static string Wrappers(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 60;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;
                IEvent eventInfo = rtEvent.Value;

                sb.AppendLine($"        , {eventName}(this, &{rtClass.Type.Wrapper.Name}::{eventInfo.AddMethod.Name}, &{rtClass.Type.Wrapper.Name}::{eventInfo.RemoveMethod.Name})");
            }

            return sb.ToString();
        }

        public static string WrappersCopyAssign(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 30;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;

                sb.AppendLine($"        {eventName} = other.{eventName};");
                sb.AppendLine($"        {eventName}.control = this;");
            }

            return sb.ToString();
        }

        public static string WrappersMoveAssign(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 30;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;

                sb.AppendLine($"        {eventName} = std::move(other.{eventName});");
            }

            return sb.ToString();
        }

        public static string WrappersMoveAssignRebind(IRTInterface rtClass)
        {
            const int AVG_EVENT_TEXT_LENGTH = 30;
            StringBuilder sb = new StringBuilder(rtClass.Events.Count * AVG_EVENT_TEXT_LENGTH);

            foreach (KeyValuePair<string, IEvent> rtEvent in rtClass.Events)
            {
                string eventName = rtEvent.Key;

                sb.AppendLine($"        {eventName}.control = this;");
            }

            return sb.ToString();
        }
    }
}
