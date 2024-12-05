from flask import Flask, request, jsonify
from google.cloud import firestore
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Initialize Firestore client
db = firestore.Client.from_service_account_json("ias-challenge-creds.json")

@app.route('/data', methods=['POST'])
def process_data():
    data = request.get_json()

    # Validate the incoming JSON
    if not data or 'industrial_zones' not in data:
        return jsonify({"error": "Invalid data format"}), 400

    try:
        # Iterate over zones in the received data
        for zone_key, zone_data in data['industrial_zones'].items():
            # Prepare Firestore reference
            collection_ref = db.collection("zone_2")  # Replace with your Firestore collection name
            doc_ref = collection_ref.document(zone_key)

            # Retrieve existing data for the zone
            existing_data = doc_ref.get().to_dict() if doc_ref.get().exists else {}

            # Append new readings to the existing data
            if 'zone_1' not in existing_data["industrial_zones"]:
                existing_data["industrial_zones"]['zone_1'] = []

            formatted_data = {
                **zone_data,
                "timestamp": zone_data.get("timestamp")
            }
            existing_data['industrial_zones']["zone_1"].append(formatted_data)

            # Update Firestore document
            doc_ref.set(existing_data)

        return jsonify({
            "status": "success",
            "message": "Data saved successfully"
        }), 201

    except Exception as e:
        print(f"Error saving data to Firestore: {e}")
        return jsonify({"error": "Failed to save data"}), 500


if __name__ == '__main__':
    app.run(debug=True, port=5001, host='0.0.0.0')
