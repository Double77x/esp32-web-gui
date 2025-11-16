#include <Arduino.h> // Needed for PROGMEM
#include "config.h"  // Good practice to include the matching header
#include "secrets.h"

// =========================================================================
// VARIABLE DEFINITIONS
// =========================================================================

const char* PARAM_INPUT = "input"; 

// =========================================================================
// WEB HTML & CERTIFICATES
// =========================================================================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <title>ESP32 WEB GUI</title>
  <style>
    :root {
      --bg: #1E1E2E; --text: #DCE0E8; --muted: #6E6C7E;
      --accent: #C6A0F6; --card: #11111b; --red: #F28FAD;
      --blue: #89B4FA;
      --card-border: rgba(110, 108, 126, .22);
      --inner-bg: rgba(0,0,0,0.1);
    }
    html, body { 
      min-height: 100vh; /* Allow body to grow */
      margin: 0; 
    }
    body {
      background: var(--bg); color: var(--text);
      font-family: system-ui, Segoe UI, Roboto, Arial, sans-serif;
      display: flex; 
      align-items: flex-start; /* FIX: Align card to top */
      justify-content: center;
      padding: 3vh 1rem; /* FIX: Add vertical padding */
      box-sizing: border-box;
    }
    .card {
      width: 500px; /* <-- UPDATED: Made card wider */
      max-width: 95vw;
      background: linear-gradient(180deg, var(--card), var(--bg));
      padding: 20px; border-radius: 12px;
      box-shadow: 0 8px 30px rgba(0, 0, 0, .6);
    }
    h1 { margin: 0 0 1rem; font-size: 20px; }
    
    /* Tabs */
    .tabs { display: flex; border-bottom: 1px solid var(--muted); margin-bottom: 1.5rem; flex-wrap: wrap; }
    .tab-link {
      padding: 0.75rem 1rem; border: none; background: none; color: var(--muted);
      font-size: 14px; font-weight: 600; cursor: pointer;
    }
    .tab-link.active { color: var(--accent); border-bottom: 2px solid var(--accent); }
    
    .tab-content { display: none; }
    .tab-content.active { display: block; }

    /* Forms */
    form { margin-bottom: 1.5rem; }
    label { display: block; font-size: 13px; color: var(--muted); margin-bottom: 6px; }
    input[type=text], input[type=password], input[type=file] {
      width: 100%; padding: 12px 10px; font-size: 16px;
      border-radius: 8px; border: 1px solid var(--card-border);
      background: transparent; color: var(--text); outline: none; box-sizing: border-box;
    }
    input[type=file] { padding: 8px; font-size: 14px; }
    button {
      width: 100%; margin-top: 12px; padding: 10px 12px;
      border-radius: 8px; border: none; background: var(--accent);
      color: var(--bg); font-weight: 600; cursor: pointer; font-size: 14px;
    }
    button:disabled { background: var(--muted); }

    /* --- Scrollable List Containers --- */
    #stock-list, #location-list {
      max-height: 200px; /* Set a max height for the lists */
      overflow-y: auto;  /* Add a scrollbar if content overflows */
      padding: 8px;
      border-radius: 6px;
      border: 1px solid var(--card-border);
      background: var(--inner-bg);
      margin-bottom: 1rem; /* Space before the form */
    }

    /* Settings List */
    h2 { font-size: 16px; margin: 1.5rem 0 0.5rem; }
    .list-item {
      display: flex; justify-content: space-between; align-items: center;
      padding: 8px 4px; /* Adjusted padding */
      border-radius: 6px; 
      margin-bottom: 6px; 
      font-size: 14px;
    }
    .remove-btn {
      color: var(--red); text-decoration: none; font-weight: bold;
      font-size: 18px; line-height: 1; cursor: pointer;
    }
    .empty-list { color: var(--muted); font-size: 13px; }
    .current-ssid { color: var(--accent); font-weight: 600; }

    /* Progress Bar */
    .progress-bar {
      width: 100%; background-color: rgba(0,0,0,0.2); border-radius: 4px;
      margin-top: 1rem; display: none;
    }
    .progress-bar-fill {
      height: 10px; width: 0%; background-color: var(--blue);
      border-radius: 4px; transition: width 0.2s;
    }
    .upload-status {
      font-size: 13px; color: var(--muted); margin-top: 6px; text-align: center;
      display: none;
    }

    /* --- NEW: Themed Scrollbar --- */
    /* For Firefox */
    * {
      scrollbar-width: thin;
      scrollbar-color: var(--muted) var(--bg);
    }
    /* For Chrome, Safari, and Edge */
    ::-webkit-scrollbar {
      width: 8px;
    }
    ::-webkit-scrollbar-track {
      background: var(--inner-bg);
      border-radius: 4px;
    }
    ::-webkit-scrollbar-thumb {
      background-color: var(--muted);
      border-radius: 4px;
    }
    ::-webkit-scrollbar-thumb:hover {
      background-color: var(--accent);
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 WEB GUI</h1>

    <!-- Tab Navigation -->
    <div class="tabs">
      <button class="tab-link active" onclick="openTab(event, 'fetch')">One-Off Fetch</button>
      <button class="tab-link" onclick="openTab(event, 'settings')">Rotation</button>
      <button class="tab-link" onclick="openTab(event, 'network')">Network</button>
      <button class="tab-link" onclick="openTab(event, 'update')">Update</button>
    </div>

    <!-- Tab 1: One-Off Fetch -->
    <div id="fetch" class="tab-content active">
      <form action="/get_stock" method="GET" onsubmit="this.ticker.value = this.ticker.value.trim().toUpperCase();">
        <label for="ticker-single">Fetch Stock (One-Time)</label>
        <input id="ticker-single" name="ticker" type="text" placeholder="e.g. AAPL" autocomplete="off" />
        <button type="submit">Fetch Stock</button>
      </form>
      <form action="/get_weather" method="GET">
        <label for="location-single">Fetch Weather (One-Time)</label>
        <input id="location-single" name="location" type="text" placeholder="e.g. London" autocomplete="off" />
        <button type="submit">Fetch Weather</button>
      </form>
    </div>

    <!-- Tab 2: Rotation Settings -->
    <div id="settings" class="tab-content">
      
      <!-- Stock Ticker List -->
      <h2>Stock Rotation List</h2>
      <div id="stock-list"><!-- Items will be injected here --></div>
      <form id="add-stock-form" action="/add_stock" method="GET" onsubmit="addItem(event, this)">
        <label for="ticker-add">Add Ticker to List</label>
        <input id="ticker-add" name="ticker" type="text" placeholder="e.g. TSLA" autocomplete="off" />
        <button type="submit">Add Stock</button>
      </form>
      
      <!-- Weather Location List -->
      <h2>Weather Rotation List</h2>
      <div id="location-list"><!-- Items will be injected here --></div>
      <form id="add-loc-form" action="/add_location" method="GET" onsubmit="addItem(event, this)">
        <label for="location-add">Add Location to List</label>
        <input id="location-add" name="location" type="text" placeholder="e.g. New York" autocomplete="off" />
        <button type="submit">Add Location</button>
      </form>

    </div>

    <!-- Tab 3: Network Settings -->
    <div id="network" class="tab-content">
      <h2>Network Settings</h2>
      <p style="font-size: 14px; color: var(--muted);">Currently connected to: <span id="current-ssid" class="current-ssid">Loading...</span></p>
      
      <form action="/connect_wifi" method="GET">
        <label for="ssid-new">New WiFi SSID</label>
        <input id="ssid-new" name="ssid" type="text" placeholder="New Network Name" autocomplete="off" />
        <label for="pass-new" style="margin-top: 1rem;">New WiFi Password</label>
        <input id="pass-new" name="pass" type="password" placeholder="New Network Password" />
        <button type="submit">Connect & Reboot</button>
      </form>
      <small style="font-size: 11px; color: var(--muted);">Device will attempt to connect. If it fails, it will revert to the default. The page will reload, and the IP may change.</small>
    </div>

    <!-- Tab 4: OTA Update -->
    <div id="update" class="tab-content">
      <h2>Firmware Update</h2>
      <p style="font-size: 14px; color: var(--muted);">Upload a new firmware (<b>.bin</b>) file. The device will restart after the update.</p>
      
      <form id="update-form" action="/update" method="POST" enctype="multipart/form-data">
        <label for="firmware-file">Firmware File (.bin)</label>
        <input id="firmware-file" name="firmware" type="file" accept=".bin" />
        <button id="update-btn" type="submit">Upload & Update</button>
      </form>
      <div class="progress-bar" id="progress-bar">
        <div class="progress-bar-fill" id="progress-bar-fill"></div>
      </div>
      <div class="upload-status" id="upload-status">Uploading... 0%</div>
    </div>

  </div>

  <script>
    function openTab(evt, tabName) {
      var i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tab-content");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
      }
      tablinks = document.getElementsByClassName("tab-link");
      for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
      }
      document.getElementById(tabName).style.display = "block";
      evt.currentTarget.className += " active";

      // If switching to settings, refresh lists
      if (tabName === 'settings' || tabName === 'network') {
        loadListsAndNetwork();
      }
    }

    // --- NEW UX FUNCTIONS ---

    // Handles adding items without a page reload
    async function addItem(event, form) {
      event.preventDefault();
      const formData = new FormData(form);
      const params = new URLSearchParams(formData);
      
      // Check if input is empty
      if (!params.values().next().value) return; 

      await fetch(form.action + '?' + params.toString());
      form.reset(); // Clear the input field
      await loadListsAndNetwork(); // Refresh the list
    }

    // Handles removing items without a page reload
    async function removeItem(event, url) {
      event.preventDefault(); // Stop page reload
      await fetch(url); // Call the remove endpoint
      await loadListsAndNetwork(); // Refresh just the lists
    }

    // Load and render lists from ESP32
    async function loadListsAndNetwork() {
      // Fetch lists
      const listResponse = await fetch('/get_lists');
      const listData = await listResponse.json();
      
      const stockListEl = document.getElementById('stock-list');
      stockListEl.innerHTML = ''; // Clear old list
      if (listData.stocks && listData.stocks.length > 0) {
        listData.stocks.forEach(ticker => {
          stockListEl.innerHTML += `
            <div class="list-item">
              <span>${ticker}</span>
              <span class="remove-btn" onclick="removeItem(event, '/remove_stock?ticker=${ticker}')" title="Remove">&times;</span>
            </div>
          `;
        });
      } else {
        stockListEl.innerHTML = '<div class="empty-list">No tickers in rotation.</div>';
      }

      const locationListEl = document.getElementById('location-list');
      locationListEl.innerHTML = ''; // Clear old list
      if (listData.locations && listData.locations.length > 0) {
        listData.locations.forEach(loc => {
          locationListEl.innerHTML += `
            <div class="list-item">
              <span>${loc}</span>
              <span class="remove-btn" onclick="removeItem(event, '/remove_location?location=${encodeURIComponent(loc)}')" title="Remove">&times;</span>
            </div>
          `;
        });
      } else {
        locationListEl.innerHTML = '<div class="empty-list">No locations in rotation.</div>';
      }

      // Fetch network status
      const networkResponse = await fetch('/get_network_status');
      const networkData = await networkResponse.json();
      if (networkData.ssid) {
        document.getElementById('current-ssid').innerText = networkData.ssid;
      }
    }

    // Initial load
    document.addEventListener('DOMContentLoaded', () => {
        // Load lists for the active tab (which is 'fetch' by default)
        // We only load when clicking the other tabs to save resources
    });
    
    // --- NEW: OTA Upload Handler ---
    const updateForm = document.getElementById('update-form');
    const updateBtn = document.getElementById('update-btn');
    const progressBar = document.getElementById('progress-bar');
    const progressBarFill = document.getElementById('progress-bar-fill');
    const uploadStatus = document.getElementById('upload-status');
    const fileInput = document.getElementById('firmware-file');

    updateForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      
      const file = fileInput.files[0];
      if (!file) {
        alert('Please select a .bin file');
        return;
      }

      updateBtn.disabled = true;
      updateBtn.innerText = 'Uploading...';
      progressBar.style.display = 'block';
      uploadStatus.style.display = 'block';
      
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);

      // Update progress
      xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
          const percent = Math.round((e.loaded / e.total) * 100);
          progressBarFill.style.width = percent + '%';
          uploadStatus.innerText = `Uploading... ${percent}%`;
        }
      });

      // Handle completion
      xhr.onload = () => {
        updateBtn.disabled = false;
        updateBtn.innerText = 'Upload & Update';
        progressBar.style.display = 'none';
        
        if (xhr.status === 200) {
          uploadStatus.style.color = 'var(--blue)';
          uploadStatus.innerHTML = 'Update successful! Rebooting...';
          // The page will be replaced by the success message
          document.body.innerHTML = xhr.responseText;
        } else {
          uploadStatus.style.color = 'var(--red)';
          uploadStatus.innerText = 'Update failed. Check console.';
          document.body.innerHTML = xhr.responseText;
        }
      };

      // Handle error
      xhr.onerror = () => {
        updateBtn.disabled = false;
        updateBtn.innerText = 'Upload & Update';
        progressBar.style.display = 'none';
        uploadStatus.style.color = 'var(--red)';
        uploadStatus.innerText = 'Upload failed. Connection error.';
      };

      const formData = new FormData();
      formData.append('firmware', file);
      xhr.send(formData);
    });
  </script>
</body>
</html>
)rawliteral";

// Finnhub CA (Google)
const char test_root_ca[] PROGMEM = R"literal(
-----BEGIN CERTIFICATE-----
MIIDejCCAmKgAwIBAgIQf+UwvzMTQ77dghYQST2KGzANBgkqhkiG9w0BAQsFADBX
MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE
CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSbootIENBMB4XDTIzMTEx
NTAzNDMyMVoXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT
GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFI0
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE83Rzp2iLYK5DuDXFgTB7S0md+8Fhzube
Rr1r1WEYNa5A3XP3iZEwWus87oV8okB2O6nGuEfYKueSkWpz6bFyOZ8pn6KY019e
WIZlD6GEZQbR3IvJx3PIjGov5cSr0R2Ko4H/MIH8MA4GA1UdDwEB/wQEAwIBhjAd
BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDwYDVR0TAQH/BAUwAwEB/zAd
BgNVHQ4EFgQUgEzW63T/STaj1dj8tT7FavCUHYwwHwYDVR0jBBgwFoAUYHtmGkUN
l8qJUC99BM00qP/8/UswNgYIKwYBBQUHAQEEKjAoMCYGCCsGAQUFBzAChhpodHRw
Oi8vaS5wa2kuZ29vZy9nc3IxLmNydDAtBgNVHR8EJjAkMCKgIKAehhxodHRwOi8v
Yy5wa2kuZ29vZy9yL2dzcjEuY3JsMBMGA1UdIAQMMAowCAYGZ4EMAQIBMA0GCSqG
SIb3DQEBCwUAA4IBAQAYQrsPBtYDh5bjP2OBDwmkoWhIDDkic574y04tfzHpn+cJ
odI2D4SseesQ6bDrarZ7C30ddLibZatoKiws3UL9xnELz4ct92vID24FfVbiI1hY
+SW6FoVHkNeWIP0GCbaM4C6uVdF5dTUsMVs/ZbzNnIdCp5Gxmx5ejvEau8otR/Cs
kGN+hr/W5GvT1tMBjgWKZ1i4//emhA1JG1BbPzoLJQvyEotc03lXjTaCzv8mEbep
8RqZ7a2CPsgRbuvTPBwcOMBBmuFeU88+FSBX6+7iP0il8b4Z0QFqIwwMHfs/L6K1
vepuoxtGzi4CZ68zJpiq1UvSqTbFJjtbD4seiMHl
-----END CERTIFICATE-----
)literal";

// Open-Meteo CA (Let's Encrypt ISRG Root X1)
const char open_meteo_ca[] PROGMEM = R"literal(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)literal";