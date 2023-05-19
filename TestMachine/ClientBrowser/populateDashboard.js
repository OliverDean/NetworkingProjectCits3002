
//using this form 
//; Individual file for 'Joel'
//; First line being user, 'name'
//; With questions being 'q - QB its from - questionID - amount of times attempted'
// filedata="Joel;q;python;a;NY-;q;c;a;---;q;c;c;Y--;q;c;d;NNN;q;python;d;NNY;q;python;f;---;q;c;h;NNN;q;python;e;Y--;q;c;o;NY-;q;c;l;NNY;"

window.onload = function() {
    // Extract the session id from the cookies
    var cookies = document.cookie.split(';');
    var sessionId = '';
    for (var i = 0; i < cookies.length; i++) {
        var cookie = cookies[i].trim();
        if (cookie.startsWith('session_id=')) {
            sessionId = cookie.substring('session_id='.length, cookie.length);
            break;
        }
    }
    if (sessionId === '') {
        console.error('Session id not found in the cookies');
        return;
    }
    
    // Use the session id to build the file name
    var fileName = '../' + sessionId + '.txt';
    
    var xhr = new XMLHttpRequest();
    xhr.open('GET', fileName, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var questions = parseQuestions(xhr.responseText); // Store the returned questions
            displayQuestions(questions); // Call a new function that will handle displaying the questions
        }
    }
    xhr.send(null);
}



// window.onload = function() {
//     var filedata = "Joel;q;python;a;NY-;q;c;a;---;q;c;c;Y--;q;c;d;NNN;q;python;d;NNY;q;python;f;---;q;c;h;NNN;q;python;e;Y--;q;c;o;NY-;q;c;l;NNY;";
//     var questions = parseQuestions(filedata); // Parse the filedata string
//     displayQuestions(questions); // Display the parsed questions
// }


function getStatus(attempts) {
    if (attempts.includes('Y')) return 'correct';
    if (attempts.includes('-')) return 'try';
    return 'incorrect';
}

function getAttemptCount(attempts) {
    var count = 0;
    for (var i = 0; i < attempts.length; i++) {
        if (attempts[i] !== '-') {
            count++;
        }
    }
    return count;
}

function parseQuestions(fileData) {
    var elements = fileData.split(';');
    var questions = [];
    var userName = elements.shift(); // Get the user's name from the first element

    // Each question is defined by four elements
    while (elements.length >= 4) {
        var questionData = elements.splice(0, 4); // Get the next four elements
        if (questionData[0] === 'q') {
            var status = getStatus(questionData[3]);
            var attemptCount = getAttemptCount(questionData[3]);
            questions.push({
                user: userName,
                type: questionData[1],
                id: questionData[2],
                status: status,
                attempt: attemptCount
            });
        }
    }
    return questions;
}

function displayQuestions(questions) { // New function to handle displaying the questions
    var questionGrid = document.querySelector('.question-grid');
    questions.forEach((item, index) => {
        var questionBox = document.createElement('div');
        questionBox.className = 'question-box';

        var questionLink = document.createElement('a');
        questionLink.className = `question-link ${item.status}`;
        questionLink.href = `question_${item.id}.html`;

        var questionNumber = document.createElement('div');
        questionNumber.className = 'question-number';
        questionNumber.textContent = `Question ${index+1}`;

        var questionType = document.createElement('div');
        questionType.className = 'question-type';
        questionType.textContent = item.type;

        var questionStatus = document.createElement('div');
        questionStatus.className = 'question-status';
        questionStatus.textContent = item.status;

        var questionAttempt = document.createElement('div');
        questionAttempt.className = 'question-attempt';
        if (item.attempt === 0)
            questionAttempt.textContent = 'Not yet Attempted';
        else
            questionAttempt.textContent = `Attempted ${item.attempt} times`;

        questionLink.appendChild(questionNumber);
        questionLink.appendChild(questionType);
        questionLink.appendChild(questionStatus);
        questionLink.appendChild(questionAttempt);

        questionBox.appendChild(questionLink);
        questionGrid.appendChild(questionBox);
    });
}
