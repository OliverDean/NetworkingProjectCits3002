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
//; either m for multi or t for coding
// Joel;
// q;m;python;a;NY-; 
// q;t;c;a;---;
// q;t;c;c;Y--;
// q;m;c;d;NNN;
// q;t;python;d;NNY;
// q;m;python;f;---;
// q;m;c;h;NNN;
// q;m;python;e;Y--;
// q;m;c;o;NY-;
// q;m;c;l;NNY;



function getStatus(attempts) {
    if (attempts.includes('Y')) return 'correct';
    if (attempts.includes('-')) return 'try';
    return 'incorrect';
}

function getQuestionType(type) {
    return type === 't' ? 'coding' : 'multi';
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
    var lines = fileData.split('\n');
    var questions = [];
    var userName = lines[0].split(';')[0];
    for (var i = 1; i < lines.length; i++) {
        var questionData = lines[i].split(';');
        if (questionData[0] === 'q') {
            var status = getStatus(questionData[4]);
            var questionType = getQuestionType(questionData[1]);
            var attemptCount = getAttemptCount(questionData[4]);
            questions.push({ 
                user: userName,
                type: questionData[2],
                questionType: questionType,
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
        questionLink.href = `question_${item.questionType}.html`;

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
