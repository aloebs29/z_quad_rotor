/**
 * @file		pressure_sensor.cpp
 * @author		Andrew Loebs
 * @brief		Source file of the pressure sensor module
 *
 */

#include "pressure_sensor.hpp"

#include <logging/log.h>

namespace z_quad_rotor {

LOG_MODULE_REGISTER(pressure_sensor, LOG_LEVEL_DBG);

// constants

// private funcions
int PressureSensor::setup_pressure_sensor(const char *dev_name)
{
    m_dev = device_get_binding(dev_name);
    if (!m_dev) {
        LOG_ERR("Pressure sensor binding failed.");
        return ENXIO;
    }
    else {
        return 0;
    }
}

int PressureSensor::setup_thread(k_thread_entry_t entry_func, struct k_thread *thread,
                                 k_thread_stack_t *stack, size_t stack_size, int priority)
{
    int err = 0;
    if (PressureSensor::THREAD_STACK_SIZE != stack_size) {
        LOG_ERR("Pressure sensor - wrong stack size, use PressureSensor::THREAD_STACK_SIZE (was: "
                "%d should be: %d).",
                stack_size, PressureSensor::THREAD_STACK_SIZE);
        err = EINVAL;
    }
    if (!err) {
        m_tid = k_thread_create(thread, stack, stack_size, entry_func, NULL, NULL, NULL, priority,
                                0, K_NO_WAIT);
        err = k_thread_name_set(m_tid, "pressure_sensor");
        if (err) {
            LOG_ERR("Unable to set pressure sensor thread name: %d", err);
        }
    }

    return err;
}

// public functions
int PressureSensor::init(const char *dev_name, k_thread_entry_t entry_func, struct k_thread *thread,
                         k_thread_stack_t *stack, size_t stack_size, int priority)
{
    int err = setup_pressure_sensor(dev_name);
    if (!err) {
        err = setup_thread(entry_func, thread, stack, stack_size, priority);
    }

    return err;
}

void PressureSensor::entry_func(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (;;) {
        // fetch data -- this will yield the thread while waiting for conversions (k_sleep) or
        // performing serial transactions
        int err = sensor_sample_fetch(m_dev);
        // store
        if (!err) {
            WriteLock<struct sensor_value> write_access = m_pressure.get_write_lock();
            err = sensor_channel_get(m_dev, SENSOR_CHAN_PRESS, &write_access.get_ref());
        }

        if (err) {
            LOG_ERR("Error reading pressure sensor: %d", err);
        }
    }
}

struct sensor_value PressureSensor::get_pressure() { return m_pressure.get_read_lock().get_var(); }

} // namespace z_quad_rotor