/* SPDX-License-Identifier: GPL-3.0-or-later
 * Include one complete production class; retain real protobuf and GLib code.
 * The idle wrapper only records the ID returned by the real scheduler.
 */
#include <glib.h>
#include <glib-object.h>

static guint scheduled_id;

static guint
fixture_schedule_idle (gint priority, GSourceFunc callback, gpointer data,
                       GDestroyNotify destroy)
{
	scheduled_id = g_idle_add_full (priority, callback, data, destroy);
	return scheduled_id;
}

#define g_idle_add_full fixture_schedule_idle
#include MEASUREMENT_SOURCE
#undef g_idle_add_full

#if MEASUREMENT_KIND == 0
#define SENSOR_TYPE SSC_TYPE_SENSOR_ACCELEROMETER
#define RESPONSE SscAccelerometerResponse
#define INIT ssc_accelerometer_response__init
#define SIZE ssc_accelerometer_response__get_packed_size
#define PACK ssc_accelerometer_response__pack
#define SET_VECTOR(msg, values) do { (msg).n_acceleration = 3; (msg).acceleration = (values); } while (0)
#elif MEASUREMENT_KIND == 1
#define SENSOR_TYPE SSC_TYPE_SENSOR_GYROSCOPE
#define RESPONSE SscGyroscopeResponse
#define INIT ssc_gyroscope_response__init
#define SIZE ssc_gyroscope_response__get_packed_size
#define PACK ssc_gyroscope_response__pack
#define SET_VECTOR(msg, values) do { (msg).n_velocity = 3; (msg).velocity = (values); } while (0)
#elif MEASUREMENT_KIND == 2
#define SENSOR_TYPE SSC_TYPE_SENSOR_LIGHT
#define RESPONSE SscLightResponse
#define INIT ssc_light_response__init
#define SIZE ssc_light_response__get_packed_size
#define PACK ssc_light_response__pack
#define SET_VECTOR(msg, values) do { (msg).n_intensity = 1; (msg).intensity = (values); } while (0)
#elif MEASUREMENT_KIND == 3
#define SENSOR_TYPE SSC_TYPE_SENSOR_PROXIMITY
#define RESPONSE SscProximityResponse
#define INIT ssc_proximity_response__init
#define SIZE ssc_proximity_response__get_packed_size
#define PACK ssc_proximity_response__pack
#elif MEASUREMENT_KIND == 4
#define SENSOR_TYPE SSC_TYPE_SENSOR_MAGNETOMETER
#define RESPONSE SscMagnetometerResponse
#define INIT ssc_magnetometer_response__init
#define SIZE ssc_magnetometer_response__get_packed_size
#define PACK ssc_magnetometer_response__pack
#define SET_VECTOR(msg, values) do { (msg).n_magnetic_field = 3; (msg).magnetic_field = (values); } while (0)
#elif MEASUREMENT_KIND == 5
#define SENSOR_TYPE SSC_TYPE_SENSOR_COMPASS
#define RESPONSE SscRotationvectorResponse
#define INIT ssc_rotationvector_response__init
#define SIZE ssc_rotationvector_response__get_packed_size
#define PACK ssc_rotationvector_response__pack
#define SET_VECTOR(msg, values) do { (msg).n_rotation = 4; (msg).rotation = (values); } while (0)
#else
#error Unknown measurement class
#endif

GObject *
fixture_measurement_sensor_new (void)
{
	/* Plain construction does not open a client, transport or sensor. */
	return g_object_new (SENSOR_TYPE,
	                     SSC_SENSOR_UID_HIGH, (guint64)42,
	                     SSC_SENSOR_UID_LOW, (guint64)43, NULL);
}

guint
fixture_measurement_queue (GObject *sensor, guint sequence, guint mode)
{
	RESPONSE response;
	GArray *buffer = g_array_new (FALSE, FALSE, 1);
	guint32 message_id = SSC_MSG_REPORT_MEASUREMENT;
#if MEASUREMENT_KIND != 3
	gfloat values[] = { 1.0f, 2.0f, 3.0f, 0.0f };
#if MEASUREMENT_KIND == 5
	values[0] = values[1] = values[2] = 0.0f;
	values[3] = 1.0f;
#endif
	(void)sequence;
#endif
	INIT (&response);
	response.accuracy = 3;
#if MEASUREMENT_KIND == 3
	response.near = sequence % 2;
	response.distance = 1;
	message_id = SSC_MSG_REPORT_MEASUREMENT_PROXIMITY;
#else
	SET_VECTOR (response, values);
#endif
	g_array_set_size (buffer, SIZE (&response));
	PACK (&response, (unsigned char *)buffer->data);
#if MEASUREMENT_KIND == 3
	if (mode == 3) {
		SscProximityResponseDavinci davinci;
		ProximityDataDavinci raw = { 0 };
		ProtobufCBinaryData bytes;
		raw.near = (gfloat)(sequence % 2);
		bytes.len = sizeof raw;
		bytes.data = (unsigned char *)&raw;
		ssc_proximity_response_davinci__init (&davinci);
		davinci.accuracy = 3;
		davinci.n_data = 1;
		davinci.data = &bytes;
		g_array_set_size (buffer, ssc_proximity_response_davinci__get_packed_size (&davinci));
		ssc_proximity_response_davinci__pack (&davinci, (unsigned char *)buffer->data);
		message_id = SSC_MSG_REPORT_MEASUREMENT;
	}
#endif
	scheduled_id = 0;
	report_received (NULL, mode == 2 ? G_MAXUINT32 : message_id,
	                 mode == 1 ? 44 : 42, 43, buffer, sensor);
	g_array_unref (buffer);
	return scheduled_id;
}
