/**
 * MyPVLog Firmware - Web UI JavaScript
 */

let currentStep = 'mode';
let selectedMode = null;
let config = {};

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    console.log('MyPVLog Firmware UI loaded');
    loadVersion();
    scanWiFiNetworks();
});

function loadVersion() {
    fetch('/api/version')
        .then(res => res.json())
        .then(data => {
            document.getElementById('version').textContent = data.version || '1.0.0';
        })
        .catch(() => {
            console.log('Could not load version');
        });
}

function selectMode(mode) {
    selectedMode = mode;
    config.mode = mode;
    console.log('Selected mode:', mode);
    nextStep('wifi');
}

function scanWiFiNetworks() {
    const ssidSelect = document.getElementById('ssid');

    fetch('/api/wifi/scan')
        .then(res => res.json())
        .then(networks => {
            ssidSelect.innerHTML = '<option value="">-- Select Network --</option>';

            networks.forEach(network => {
                const option = document.createElement('option');
                option.value = network.ssid;
                option.textContent = `${network.ssid} (${network.rssi} dBm)`;
                ssidSelect.appendChild(option);
            });
        })
        .catch(() => {
            ssidSelect.innerHTML = '<option value="">Scan failed - enter manually</option>';
        });
}

function nextStep(step) {
    const steps = document.querySelectorAll('.step');
    steps.forEach(s => s.classList.remove('active'));

    if (step === 'mqtt-generic' && selectedMode === 'generic') {
        document.getElementById('step-mqtt-generic').classList.add('active');
    } else if (step === 'mypvlog-login' && selectedMode === 'mypvlog') {
        document.getElementById('step-mypvlog-login').classList.add('active');
    } else {
        document.getElementById('step-' + step).classList.add('active');
    }

    currentStep = step;
    window.scrollTo(0, 0);
}

function prevStep() {
    const stepFlow = {
        'wifi': 'mode',
        'mqtt-generic': 'wifi',
        'mypvlog-login': 'wifi'
    };

    const prevStep = stepFlow[currentStep];
    if (prevStep) {
        nextStep(prevStep);
    }
}

function toggleSSL() {
    const sslCheckbox = document.getElementById('mqtt-ssl');
    const portInput = document.getElementById('mqtt-port');

    if (sslCheckbox.checked) {
        portInput.value = '8883';
    } else {
        portInput.value = '1883';
    }
}

function showLoading(message) {
    const loading = document.getElementById('loading');
    const loadingText = document.getElementById('loading-text');

    loadingText.textContent = message;
    loading.classList.remove('hidden');
}

function hideLoading() {
    document.getElementById('loading').classList.add('hidden');
}

function showError(message) {
    alert('Error: ' + message);
    hideLoading();
}

function showComplete(message, nextSteps) {
    document.getElementById('complete-message').textContent = message;

    const nextStepsList = document.getElementById('next-steps');
    nextStepsList.innerHTML = '';
    nextSteps.forEach(step => {
        const li = document.createElement('li');
        li.textContent = step;
        nextStepsList.appendChild(li);
    });

    nextStep('complete');
    hideLoading();
}

// WiFi Form Submission
document.getElementById('wifi-form').addEventListener('submit', function(e) {
    e.preventDefault();

    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    config.wifi = { ssid, password };

    showLoading('Connecting to WiFi...');

    fetch('/api/wifi/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password })
    })
    .then(res => res.json())
    .then(data => {
        hideLoading();

        if (data.success) {
            if (selectedMode === 'generic') {
                nextStep('mqtt-generic');
            } else {
                nextStep('mypvlog-login');
            }
        } else {
            showError('WiFi connection failed: ' + data.error);
        }
    })
    .catch(err => {
        showError('WiFi connection error: ' + err.message);
    });
});

// Generic MQTT Form Submission
document.getElementById('mqtt-form').addEventListener('submit', function(e) {
    e.preventDefault();

    const mqttConfig = {
        host: document.getElementById('mqtt-host').value,
        port: parseInt(document.getElementById('mqtt-port').value),
        ssl: document.getElementById('mqtt-ssl').checked,
        username: document.getElementById('mqtt-user').value,
        password: document.getElementById('mqtt-pass').value,
        topic: document.getElementById('mqtt-topic').value
    };

    config.mqtt = mqttConfig;

    showLoading('Testing MQTT connection...');

    fetch('/api/mqtt/configure', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(mqttConfig)
    })
    .then(res => res.json())
    .then(data => {
        if (data.success) {
            showComplete(
                'Generic MQTT mode configured successfully!',
                [
                    'Inverter data will be published to your MQTT broker',
                    'Topic format: ' + mqttConfig.topic + '/{dtu_id}/{inverter_serial}',
                    'Device will reboot and start polling inverters',
                    'Configure inverters from the dashboard'
                ]
            );
        } else {
            showError('MQTT configuration failed: ' + data.error);
        }
    })
    .catch(err => {
        showError('MQTT configuration error: ' + err.message);
    });
});

// MyPVLog Login Form Submission
document.getElementById('login-form').addEventListener('submit', function(e) {
    e.preventDefault();

    const email = document.getElementById('email').value;
    const password = document.getElementById('login-password').value;

    showLoading('Signing in to MyPVLog.net...');

    fetch('/api/mypvlog/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password })
    })
    .then(res => res.json())
    .then(data => {
        if (data.success) {
            // Proceed with provisioning
            provisionDevice(data.token);
        } else {
            showError('Login failed: ' + data.error);
        }
    })
    .catch(err => {
        showError('Login error: ' + err.message);
    });
});

function loginGoogle() {
    showLoading('Opening Google Sign-In...');

    fetch('/api/mypvlog/oauth/google')
        .then(res => res.json())
        .then(data => {
            if (data.url) {
                window.open(data.url, '_blank', 'width=500,height=600');
                pollOAuthStatus();
            } else {
                showError('OAuth initiation failed');
            }
        })
        .catch(err => {
            showError('OAuth error: ' + err.message);
        });
}

function pollOAuthStatus() {
    const interval = setInterval(() => {
        fetch('/api/mypvlog/oauth/status')
            .then(res => res.json())
            .then(data => {
                if (data.authenticated) {
                    clearInterval(interval);
                    provisionDevice(data.token);
                }
            })
            .catch(() => {
                // Continue polling
            });
    }, 2000);

    // Stop polling after 5 minutes
    setTimeout(() => {
        clearInterval(interval);
        hideLoading();
    }, 300000);
}

function provisionDevice(token) {
    showLoading('Registering device with MyPVLog.net...');

    fetch('/api/mypvlog/provision', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ token })
    })
    .then(res => res.json())
    .then(data => {
        if (data.success) {
            showComplete(
                'MyPVLog Direct mode configured successfully!',
                [
                    'DTU registered: ' + data.dtu_id,
                    'Found ' + data.inverter_count + ' inverter(s)',
                    'View your dashboard at mypvlog.net',
                    'Download the mobile app for on-the-go monitoring',
                    'Device will reboot and start polling inverters'
                ]
            );
        } else {
            showError('Device provisioning failed: ' + data.error);
        }
    })
    .catch(err => {
        showError('Provisioning error: ' + err.message);
    });
}

function openSignup() {
    window.open('https://mypvlog.net/register', '_blank');
}
