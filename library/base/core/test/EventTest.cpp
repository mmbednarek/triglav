#include "triglav/testing_core/GTest.hpp"

#include "triglav/Event.hpp"

using namespace triglav::name_literals;

class Receiver
{
 public:
   void on_example(const float value)
   {
      m_received_value = value + m_base_value;
   }

   void on_values(const int a, const int b)
   {
      m_received_value = static_cast<float>(a + b);
   }

   void get_value(float& out_value)
   {
      out_value = m_received_value;
   }

   float m_received_value = 0.0f;
   float m_base_value = 0.0f;
};


TEST(EventTest, SingleValue)
{
   triglav::Event<float> example;

   Receiver receiver;
   triglav::EventManager::the().register_callback(example.event_id(), &receiver, reinterpret_cast<void*>(+[](void* handle, float value) {
                                                     static_cast<Receiver*>(handle)->on_example(value);
                                                  }));

   ASSERT_EQ(receiver.m_received_value, 0.0f);
   example.publish(4.0f);
   ASSERT_EQ(receiver.m_received_value, 4.0f);
}

TEST(EventTest, MultiValue)
{
   triglav::Event<float> example;

   Receiver receiver1;
   receiver1.m_base_value = 2.0f;
   Receiver receiver2;
   receiver2.m_base_value = 3.0f;
   triglav::EventManager::the().register_callback(example.event_id(), &receiver1, reinterpret_cast<void*>(+[](void* handle, float value) {
                                                     static_cast<Receiver*>(handle)->on_example(value);
                                                  }));
   triglav::EventManager::the().register_callback(example.event_id(), &receiver2, reinterpret_cast<void*>(+[](void* handle, float value) {
                                                     static_cast<Receiver*>(handle)->on_example(value);
                                                  }));

   ASSERT_EQ(receiver1.m_received_value, 0.0f);
   ASSERT_EQ(receiver2.m_received_value, 0.0f);
   example.publish(4.0f);
   ASSERT_EQ(receiver1.m_received_value, 6.0f);
   ASSERT_EQ(receiver2.m_received_value, 7.0f);
}

TEST(EventTest, Sink)
{
   triglav::Event<int, int> example;

   Receiver receiver;

   {
      auto sink = example.connect<&Receiver::on_values>(receiver);
      example.publish(21, 37);
      ASSERT_EQ(receiver.m_received_value, 58.0f);
   }

   example.publish(10, 10);
   // No change, sink was removed
   ASSERT_EQ(receiver.m_received_value, 58.0f);

   auto sink = example.connect<&Receiver::on_values>(receiver);

   // Created new connection
   example.publish(20, 20);
   ASSERT_EQ(receiver.m_received_value, 40.0f);

   sink.release();

   // Sink released, no change
   example.publish(30, 20);
   ASSERT_EQ(receiver.m_received_value, 40.0f);
}

TEST(EventTest, ManyReceivers)
{
   triglav::Event<float> example;

   std::array<Receiver, 10> receivers{};
   std::array<triglav::Sink, 10> sinks{};

   for (int i = 0; i < 10; ++i) {
      receivers[i].m_base_value = static_cast<float>(i);
      sinks[i] = example.connect<&Receiver::on_example>(receivers[i]);
   }

   example.publish(12.34f);

   for (int i = 0; i < 10; ++i) {
      ASSERT_EQ(receivers[i].m_received_value, 12.34f + static_cast<float>(i));
   }
}

TEST(EventTest, Reference)
{
   triglav::Event<float&> example;

   Receiver receiver;
   auto sink = example.connect<&Receiver::get_value>(receiver);

   receiver.m_received_value = 12.34f;
   float result;
   example.publish(result);

   ASSERT_EQ(result, 12.34f);
}
