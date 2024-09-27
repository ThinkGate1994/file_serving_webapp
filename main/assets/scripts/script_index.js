var totalCount = 0;
var currentCount = 0;
var mode = "";

async function extractFiles() {
    const fileInput = document.getElementById('fileInput');
    const outputDiv = document.getElementById('output');
    const progressDiv = document.getElementById('progress');
    const countProgress = document.getElementById('stat');

    progressDiv.textContent = '';
    countProgress.textContent = '';
    outputDiv.innerHTML = '';

    if (!fileInput || !fileInput.files || fileInput.files.length === 0) {
        outputDiv.innerHTML = 'Please select a zip file.';
        return;
    }

    const fileName = fileInput.files[0].name;
    mode = fileName.split("-");
    const zipFile = fileInput.files[0];
    const fileReader = new FileReader();

    if (!(mode[0] == "allf" || mode[0] == "weba" || mode[0] == "conf")) {
        outputDiv.innerHTML = 'Error: The zip file is not compatible.';
        return;
    }

    fileReader.onload = function (event) {
        const arrayBuffer = event.target.result;
        JSZip.loadAsync(arrayBuffer).then(function (zip) {
            if (Object.keys(zip.files).length === 0) {
                outputDiv.innerHTML = 'Error: The zip file is empty.';
                return;
            }

            const promises = [];
            const fileList = [];

            zip.forEach(function (relativePath, zipEntry) {
                if (!zipEntry.dir) {
                    promises.push(zipEntry.async('uint8array').then(function (fileData) {
                        const fileBlob = new Blob([fileData], { type: 'application/octet-stream' });
                        const downloadLink = document.createElement('a');
                        downloadLink.href = URL.createObjectURL(fileBlob);
                        downloadLink.download = zipEntry.name;
                        downloadLink.textContent = zipEntry.name;
                        outputDiv.appendChild(downloadLink);
                        outputDiv.appendChild(document.createElement('br'));
                        fileList.push({ name: zipEntry.name, data: fileData });
                    }));
                }
            });

            Promise.all(promises).then(function () {
                if (fileList.length > 0) {
                    outputDiv.innerHTML += '<p>Extraction complete!</p>';
                    upload(fileList);
                } else {
                    outputDiv.innerHTML = 'Error: The zip file does not contain any valid files.';
                }
            });
        }).catch(function (error) {
            outputDiv.innerHTML = 'Error extracting files: ' + error.message;
        });
    };

    fileReader.onerror = function () {
        outputDiv.innerHTML = 'Error reading the zip file.';
    };

    fileReader.readAsArrayBuffer(zipFile);
}

async function upload(fileList) {
    totalCount = fileList.length;
    currentCount = 0;
    try {
        await uploadFilesRecursive(fileList);
        await sendPostRequestWhenUploadComplete();
        currentCount = totalCount;
        const countProgress = document.getElementById('stat');
        countProgress.textContent = 'File: ' + currentCount + ' of ' + totalCount;
    } catch (error) {
        console.error('Error uploading files:', error);
    }
}

async function uploadFilesRecursive(fileList) {
    if (fileList.length === 0) {
        return;
    }
    const file = fileList.shift();
    await uploadFile(file);
    await uploadFilesRecursive(fileList);
}

async function uploadFile(file) {
    let newFilePath;
    if (mode[0] == "allf") {
        const lastSlashIndex = file.name.lastIndexOf("/");
        let folderPath, newFolderPath, prefix;
        folderPath = file.name.substring(file.name.indexOf("/") + 1, lastSlashIndex);
        if (folderPath == "/") {
            newFolderPath = '_new';
        } else {
            prefix = folderPath.substring(0, folderPath.indexOf("/"));
            newFolderPath = prefix + "_new/" + folderPath.substring(folderPath.indexOf("/") + 1);
        }
        newFilePath = newFolderPath + file.name.substring(lastSlashIndex);
    } else if (mode[0] == "weba" || mode[0] == "conf") {
        newFilePath = file.name.substring(file.name.indexOf("/") + 1) + "_new";
    } else {
        return;
    }
    const pathToSend = '/upload/' + newFilePath;
    const xhr = new XMLHttpRequest();

    return new Promise((resolve, reject) => {
        xhr.upload.addEventListener('progress', function (event) {
            if (event.lengthComputable) {
                const progress = Math.round((event.loaded / event.total) * 100);
                if (progress == 100 && currentCount < totalCount) {
                    currentCount++;
                }
                updateProgress(file.name, progress);
            }
        });

        xhr.addEventListener('load', function () {
            if (xhr.status === 200) {
                console.log(file.name + ' uploaded successfully.');
                resolve();
            } else {
                console.error(file.name + ' upload failed with status:', xhr.status);
                reject(new Error('Upload failed'));
            }
        });

        xhr.addEventListener('error', function () {
            console.error('Error uploading ' + file.name);
            reject(new Error('Upload error'));
        });

        xhr.open('POST', pathToSend);
        xhr.send(new Blob([file.data], { type: 'application/octet-stream' }));
    });
}

async function sendPostRequestWhenUploadComplete() {
    var response;

    try {
        if (mode[0] == "allf") {
            response = await fetch('/clean/allf', { method: 'POST' });
        } else if (mode[0] == "conf") {
            response = await fetch('/clean/conf', { method: 'POST' });
        } else if (mode[0] == "weba") {
            response = await fetch('/clean/weba', { method: 'POST' });
        }

        if (response.ok) {
            console.log("POST request sent successfully!");
            const progressBar = document.getElementById('fileProgressBar');
            const successMessage = document.getElementById('uploadSuccessMessage'); // Show success message

            progressBar.style.width = `100%`;
            progressBar.textContent = `100%`;
            successMessage.style.display = 'block';

            // Hide the success messafe after 3 seconds
            setTimeout(() => {
                successMessage.style.display = 'none';
            }, 5000); //Adjust the time as needed
        } else {
            console.log(response.status + ' Error!' + response.statusText);
        }
    } catch (error) {
        console.log('Error sending POST request:', error);
    }
}

function updateProgress(filename, progress) {
    const countProgress = document.getElementById('stat');
    const progressDiv = document.getElementById('progress');
    const progressBar = document.getElementById('fileProgressBar');

    // Calculate the progress based on the number of files uploaded
    const fileProgress = Math.round((currentCount / totalCount) * 100);

    // Update the progress bar width and text inside it
    progressBar.style.width = `${fileProgress}%`;
    progressBar.textContent = `${fileProgress}%`;

    // Update the progress text
    countProgress.textContent = 'File: ' + currentCount + ' of ' + totalCount;
    progressDiv.textContent = `File Upload Progress: ${progress}%`;
}

document.addEventListener('DOMContentLoaded', function () {
    var readTab = document.getElementById('Read-tab');

    // Add event listener for tab change
    readTab.addEventListener('shown.bs.tab', function (event) {
        if (event.target.id === 'Read-tab') {
            loadDirectory('/dir/sdcard');
        }
    });
});

let currentPath = '/dir/sdcard'; // Initialize the current path as the root directory
function loadDirectory(path) {
    currentPath = path; // Update the current path

    fetch(path, {
        method: 'GET',
        headers: {
            'Content-Type': 'application/json',
        }
    })
        .then(response => response.json())
        .then(data => {
            console.log('Data from', path, ':', data);

            const outputDiv = document.getElementById('directory');
            outputDiv.innerHTML = '';

            if (data.length === 0) {
                outputDiv.innerHTML = 'SD card is empty.';
            }
            else {
                //Create a table to display the directory content
                const table = document.createElement('table');
                table.className = 'table table-striped';

                //Create table header
                const thead = document.createElement('thead');
                const headerRow = document.createElement('tr');
                const nameHeader = document.createElement('th');
                nameHeader.textContent = 'Name';
                const typeHeader = document.createElement('th');
                typeHeader.textContent = 'Type';
                const sizeHeader = document.createElement('th');
                sizeHeader.textContent = 'Size(KB)';
                const downloadHeader = document.createElement('th');
                downloadHeader.textContent = 'Download';
                headerRow.appendChild(nameHeader);
                headerRow.appendChild(typeHeader);
                headerRow.appendChild(sizeHeader);
                headerRow.appendChild(downloadHeader);
                thead.appendChild(headerRow);
                table.appendChild(thead);

                //Create table body
                const tbody = document.createElement('tbody');
                data.forEach(item => {
                    const row = document.createElement('tr');

                    const nameCell = document.createElement('td');
                    nameCell.textContent = item.name;

                    const typeCell = document.createElement('td');
                    typeCell.textContent = item.type;

                    const sizeCell = document.createElement('td');
                    sizeCell.textContent = item.size ? `${item.size} KB` : '';

                    const downloadCell = document.createElement('td');
                    if (item.type === 'file') {
                        const downloadButton = document.createElement('button');
                        downloadButton.textContent = 'Download';
                        downloadButton.className = 'btn btn-success btn-sm';
                        downloadButton.onclick = () => downloadFile(`${path}/${item.name}`);
                        downloadCell.appendChild(downloadButton);
                    }

                    row.appendChild(nameCell);
                    row.appendChild(typeCell);
                    row.appendChild(sizeCell);
                    row.appendChild(downloadCell);

                    //Add click event for directories
                    if (item.type === 'directory') {
                        nameCell.style.cursor = 'pointer';// change cursor to pointer
                        nameCell.onclick = () => loadDirectory(`${path}/${item.name}`);
                        nameCell.style.color = 'blue';
                    }

                    tbody.appendChild(row);
                });
                table.appendChild(tbody);
                outputDiv.appendChild(table);
            }

            // Check if we're not in the root directory to show the Back button
            if (currentPath !== '/dir/sdcard') {
                const backButton = document.createElement('button');
                backButton.textContent = 'Back';
                backButton.className = 'btn btn-secondary mb-2'; // Bootstrap class for styling
                backButton.onclick = () => navigateBack(); // Back button functionality
                outputDiv.appendChild(backButton);
            }

            // Add "Download All" button at the root directory
            if (currentPath === '/dir/sdcard' && data.length !== 0) {
                const downloadAllButton = document.createElement('button');
                downloadAllButton.textContent = 'Download All Files';
                downloadAllButton.className = 'btn btn-primary mb-2';
                downloadAllButton.onclick = downloadAllFiles;
                outputDiv.appendChild(downloadAllButton);
            }
        })
        .catch(error => {
            console.error('Error fetching data from', path, ':', error);
        });
}

// Function to navigate back to the parent directory
function navigateBack() {
    if (currentPath !== '/dir/sdcard') {
        // Remove the last part of the path to navigate back
        currentPath = currentPath.substring(0, currentPath.lastIndexOf('/'));
        if (currentPath === '') {
            currentPath = '/dir/sdcard'; // Make sure we reset to the root if needed
        }
        loadDirectory(currentPath); // Load the parent directory
    }
}

// Function to download an individual file
function downloadFile(filePath) {
    console.log('download file path: ', filePath);
    const link = document.createElement('a');
    link.href = `/download${filePath.replace('/dir/sdcard', '')}`; // Build the download URL based on file path
    link.download = filePath.split('/').pop(); // Extract the file name for download
    link.click();
}

// Function to download all files in the root directory
// function downloadAllFiles() {
//     const link = document.createElement('a');
//     link.href = '/download_all'; // This is the endpoint for downloading all files (needs implementation on ESP32 side)
//     link.download = 'all_files.zip'; // File name for the downloaded zip
//     link.click();
// }

// async function downloadAllFiles() {
//     const response = await fetch('/list-files');  // Endpoint to list files
//     const fileList = await response.json();

//     let zip = new JSZip();

//     // Loop through each file
//     for (let filePath of fileList) {
//         let fileResponse = await fetch(filePath);
//         let fileData = await fileResponse.blob();

//         // Add file to the ZIP
//         zip.file(filePath.substring(filePath.lastIndexOf('/') + 1), fileData);
//     }

//     // Generate ZIP file
//     zip.generateAsync({ type: "blob" }).then(function(content) {
//         // Download ZIP file
//         let link = document.createElement("a");
//         link.href = URL.createObjectURL(content);
//         link.download = "all_files.zip";
//         document.body.appendChild(link);
//         link.click();
//         document.body.removeChild(link);
//     });
// }


async function downloadAllFiles() {
    try {
        const response = await fetch('/list-files');  // Fetch list of files recursively
        const fileList = await response.json();

        console.log("File list received: ", fileList);  // Debugging file list

        let zip = new JSZip();

        // Loop through all files (including files in subdirectories)
        for (let filePath of fileList) {
            try {
                console.log("Fetching file: ", filePath);  // Debugging file fetch

                let fileResponse = await fetch(filePath);
                if (!fileResponse.ok) {
                    console.error("Failed to fetch file: ", filePath);  // Error handling
                    continue;
                }

                let fileData = await fileResponse.blob();
                console.log("Adding file to ZIP: ", filePath);  // Debugging zip addition

                // Preserve the directory structure when adding files to the ZIP
                let relativePath = filePath.replace("/sdcard/", ""); // Remove base path
                zip.file(relativePath, fileData);

            } catch (fileError) {
                console.error("Error fetching file: ", filePath, fileError);  // Error handling
            }
        }

        // Generate ZIP and trigger download
        zip.generateAsync({ type: "blob" }).then(function (content) {
            console.log("ZIP generation complete.");  // Debugging zip completion
            let link = document.createElement("a");
            link.href = URL.createObjectURL(content);
            link.download = "all_files.zip";
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }).catch(function (zipError) {
            console.error("Error generating ZIP: ", zipError);  // Error handling
        });

    } catch (error) {
        console.error("Error fetching file list: ", error);  // Error handling
    }
}

function clear_flash() {
    // Show the buffering spinner
    document.getElementById('buffering').style.display = 'block';

    fetch('/api/clear_flash', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
    })
        .then(response => {
            if (!response.ok) {
                // Hide the buffering spinner
                document.getElementById('buffering').style.display = 'none';

                //show fail message as pop-up
                const failMessage = document.getElementById('f_fail');
                failMessage.style.display = 'block';

                //Hide the fail message after 3 seconds
                setTimeout(() => {
                    failMessage.style.display = 'none';
                }, 5000); //Adjust the time as needed

                throw new Error('Fail to clear the flash');
            }
            return response.json();
        })
        .then(responseData => {
            console.log('Server response:', responseData);

            // Hide the buffering spinner
            document.getElementById('buffering').style.display = 'none';

            if (responseData.status == "success") {
                // show success message as a pop-up
                const successMessage = document.getElementById('f_success');
                successMessage.style.display = 'block';

                // Hide the success messafe after 3 seconds
                setTimeout(() => {
                    successMessage.style.display = 'none';
                }, 5000); //Adjust the time as needed
            }
            else {
                //show fail message as pop-up
                const failMessage = document.getElementById('f_fail');
                failMessage.style.display = 'block';

                //Hide the fail message after 3 seconds
                setTimeout(() => {
                    failMessage.style.display = 'none';
                }, 5000); //Adjust the time as needed
            }

        })
        .catch(error => {
            console.error('Error:', error);
        });
}

