window.onload = function() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'userTest.txt', true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200)
            parseQuestions(xhr.responseText);
    }
    xhr.send(null);
}

//using this form 
//; Individual file for 'Joel'
//; First line being user, 'name'
//; With questions being 'q - QB its from - questionID - amount of times attempted'

// Joel;
// q;python;a;NY-; 
// q;c;a;---;
// q;c;c;Y--;
// q;c;d;NNN;
// q;python;d;NNY;
// q;python;f;---;
// q;c;h;NNN;
// q;python;e;Y--;
// q;c;o;NY-;
// q;c;l;NNY;

/**
 * Determine the status of a question based on the attempts string
 * @param {string} attempts - The attempts string for a question
 * @returns {string} - The status of the question: 'correct', 'try', or 'incorrect'
 */
function getStatus(attempts) {
    if (attempts.includes('Y')) return 'correct';
    if (attempts.includes('-')) return 'try';
    return 'incorrect';
}

/**
 * Count the number of attempts made on a question
 * @param {string} attempts - The attempts string for a question
 * @returns {number} - The number of attempts made on the question
 */
function getAttemptCount(attempts) {
    var count = 0;
    for (var i = 0; i < attempts.length; i++) {
        if (attempts[i] !== '-') {
            count++;
        }
    }
    return count;
}

/**
 * Parse the questions from the file data
 * @param {string} fileData - The file data to be parsed
 */
function parseQuestions(fileData) {
    var lines = fileData.split('\n');
    var questions = [];
    var userName = lines[0].split(';')[0];
    for (var i = 1; i < lines.length; i++) {
        var questionData = lines[i].split(';');
        if (questionData[0] === 'q') {
            var status = getStatus(questionData[3]);
            var attemptCount = getAttemptCount(questionData[3]);
            questions.push({ 
                user: userName,
                type: questionData[1],
                status: status,
                attempt: attemptCount
            });
        }
    }

    var questionGrid = document.querySelector('.question-grid');
    questions.forEach((item, index) => {
        var questionBox = document.createElement('div');
        questionBox.className = 'question-box';

        var questionLink = document.createElement('a');
        questionLink.className = `question-link ${item.status}`;
        questionLink.href = `question_${index + 1}.html`;

        var questionNumber = document.createElement('div');
        questionNumber.className = 'question-number';
        questionNumber.textContent = `Question ${index + 1}`;

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
