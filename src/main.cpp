#include <Arduino.h>
#include <WiFi.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

/* ===================== CONFIG ===================== */

// WiFi credentials
char ssid[] = "PRABHU";
char password[] = "kee129..";

// micro-ROS Agent (PC running micro-ROS agent)
IPAddress agent_ip(10,52,106,239);   // CHANGE THIS
uint16_t agent_port = 8888;

// MQ-3 sensor pin
#define MQ3_PIN 35   // ADC pin (ESP32)

/* ================================================== */

rcl_publisher_t publisher;
std_msgs__msg__Int32 mq3_msg;

rcl_timer_t timer;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

/* ===================== MACROS ===================== */

#define RCCHECK(fn) { rcl_ret_t rc = fn; if (rc != RCL_RET_OK) error_loop(); }
#define RCSOFTCHECK(fn) { rcl_ret_t rc = fn; (void) rc; }

/* ===================== ERROR LOOP ===================== */

void error_loop()
{
  while (1)
  {
    delay(100);
  }
}

/* ===================== TIMER CALLBACK ===================== */

void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
  (void) last_call_time;

  if (timer != NULL)
  {
    int mq3_value = analogRead(MQ3_PIN);
    mq3_msg.data = mq3_value;

    RCSOFTCHECK(rcl_publish(&publisher, &mq3_msg, NULL));
  }
}

/* ===================== SETUP ===================== */

void setup()
{
  Serial.begin(115200);
  delay(2000);

  // Configure WiFi micro-ROS transport
  set_microros_wifi_transports(
    ssid,
    password,
    agent_ip,
    agent_port
  );

  delay(2000);

  // ADC setup
  analogReadResolution(12); // 0–4095
  pinMode(MQ3_PIN, INPUT);

  allocator = rcl_get_default_allocator();

  // Initialize micro-ROS support
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create node
  RCCHECK(
    rclc_node_init_default(
      &node,
      "mq3_gas_sensor_node",
      "",
      &support
    )
  );

  // Create publisher
  RCCHECK(
    rclc_publisher_init_default(
      &publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "mq3/raw"
    )
  );

  // Create timer (1 Hz)
  const unsigned int timer_timeout = 1000;
  RCCHECK(
    rclc_timer_init_default(
      &timer,
      &support,
      RCL_MS_TO_NS(timer_timeout),
      timer_callback
    )
  );

  // Create executor
  RCCHECK(
    rclc_executor_init(
      &executor,
      &support.context,
      1,
      &allocator
    )
  );

  RCCHECK(
    rclc_executor_add_timer(&executor, &timer)
  );
}

/* ===================== LOOP ===================== */

void loop()
{
  RCSOFTCHECK(
    rclc_executor_spin_some(
      &executor,
      RCL_MS_TO_NS(100)
    )
  );

  delay(10);
}

