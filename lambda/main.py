import boto3
import json
from datetime import datetime


def lambda_handler(event, context):
    client = boto3.client('ses')
    if 'Records' in event:
        for record in event['Records']:
            iot_data = json.loads(record['body'])
            timestamp = iot_data['ts']
            readable_time = datetime.fromtimestamp(int(timestamp/1000)).strftime('%Y-%m-%d %H:%M:%S')
            message_body = 'Temperature seems high. Latest recorded temperature is <strong>{0}</strong> on <strong>{1}</strong>.<br><br>Heat Index: {2}<br>Humidity: {3}<br><br>Notification sent from <strong>{4}</strong>.'.format(
                iot_data['temperature'], readable_time, iot_data['heat_index'], iot_data['humidity'], iot_data['device_id'])
            client.send_email(
                Source='<SENDER EMAIL>',
                Destination={
                    'ToAddresses': ['RECIEVER EMAIL'],
                },
                Message={
                    'Subject': {'Data': 'Temperature Alert! - {0}'.format(readable_time)},
                    'Body': {
                        'Html': {'Data': message_body}
                    }
                },
            )