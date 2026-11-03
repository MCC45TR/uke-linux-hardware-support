/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compile the actual upstream C translation unit, replacing only transport.
 * The C++ driver uses real GObject, GTask, signals and generated protobuf code.
 */
#define ssc_client_send fixture_client_send
#define ssc_client_send_finish fixture_client_send_finish
#define ssc_client_new fixture_client_new
#define ssc_client_new_finish fixture_client_new_finish
#include LIFECYCLE_SOURCE

SSCSensor *
fixture_sensor_new (SSCClient **client)
{
	SSCSensor *sensor = g_object_new (SSC_TYPE_SENSOR, NULL);
	SSCSensorPrivate *priv = ssc_sensor_get_instance_private (sensor);
	priv->client = g_object_new (SSC_TYPE_CLIENT, NULL);
	priv->available = TRUE;
	priv->uid_high = 42;
	priv->uid_low = 43;
	priv->stream_type = SSC_STREAM_TYPE_ON_CHANGE;
	priv->data_type = g_strdup ("proximity");
	*client = g_object_ref (priv->client);
	return sensor;
}

void
fixture_measurement (SSCClient *client)
{
	GArray *buf = g_array_new (FALSE, FALSE, 1);
	g_signal_emit_by_name (client, "report", SSC_MSG_REPORT_MEASUREMENT,
			      (guint64)42, (guint64)43, buf);
	g_array_unref (buf);
}

void
fixture_discovery (SSCClient *client, gboolean registry, gboolean found)
{
	SscSuidResponse msg;
	SscUid uid;
	SscUid *uids[] = { &uid };
	GArray *buf = g_array_new (FALSE, FALSE, 1);
	ssc_suid_response__init (&msg);
	ssc_uid__init (&uid);
	uid.high = 42;
	uid.low = 43;
	msg.data_type = registry ? "registry" : "proximity";
	msg.n_uid = found ? 1 : 0;
	msg.uid = uids;
	g_array_set_size (buf, ssc_suid_response__get_packed_size (&msg));
	ssc_suid_response__pack (&msg, (unsigned char *)buf->data);
	g_signal_emit_by_name (client, "report", SSC_MSG_RESPONSE_SUID,
			      SSC_SENSOR_UID_SUID_HIGH, SSC_SENSOR_UID_SUID_LOW, buf);
	g_array_unref (buf);
}

void
fixture_attributes (SSCClient *client)
{
	SscAttrResponse msg;
	GArray *buf = g_array_new (FALSE, FALSE, 1);
	ssc_attr_response__init (&msg);
	g_array_set_size (buf, ssc_attr_response__get_packed_size (&msg));
	ssc_attr_response__pack (&msg, (unsigned char *)buf->data);
	g_signal_emit_by_name (client, "report", SSC_MSG_RESPONSE_GET_ATTRIBUTES,
			      (guint64)42, (guint64)43, buf);
	g_array_unref (buf);
}
