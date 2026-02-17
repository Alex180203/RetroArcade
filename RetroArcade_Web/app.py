from flask import Flask, render_template, request, redirect, url_for, flash, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager, UserMixin, login_user, login_required, logout_user, current_user
from flask_socketio import SocketIO, emit
from datetime import datetime

app = Flask(__name__)
app.config['SECRET_KEY'] = 'arcade_master_key_999'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///arcade.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
socketio = SocketIO(app, cors_allowed_origins="*")
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'


CONTROLLER_STATE = "disconnected" 

class User(UserMixin, db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(150), unique=True, nullable=False)
    password = db.Column(db.String(150), nullable=False)
    snake_color = db.Column(db.String(20), default="#00FF00")
    scores = db.relationship('Score', backref='player', lazy=True)

class Score(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    game_name = db.Column(db.String(50), nullable=False)
    score = db.Column(db.Integer, nullable=False)
    date = db.Column(db.DateTime, default=datetime.utcnow)
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'), nullable=False)

@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))

@socketio.on('connect')
def handle_connect():
    """
    When a browser opens the Hub, immediately tell it the current state.
    This fixes the 'Locked even if connected' bug on refresh.
    """
    print(f"Client connected. Sending current state: {CONTROLLER_STATE}")
    emit('controller_status', {'action': CONTROLLER_STATE})

@app.route('/move', methods=['GET', 'POST'])
def handle_move():
    global CONTROLLER_STATE
    
    if request.method == 'GET':
        return jsonify({"status": "online", "current_state": CONTROLLER_STATE})

    data = request.json
    if data:
        if 'action' in data:
            CONTROLLER_STATE = data['action'] 
            print(f"--> STATE UPDATE: {CONTROLLER_STATE}")
            
            socketio.emit('controller_status', {'action': CONTROLLER_STATE}) 
            return jsonify({"status": "success", "mode": CONTROLLER_STATE})

        socketio.emit('game_input', data)
        return jsonify({"status": "success"}), 200
    
    return jsonify({"status": "error"}), 400

@app.route('/')
def home():
    if current_user.is_authenticated:
        return redirect(url_for('hub'))
    return redirect(url_for('login'))

@app.route('/hub')
@login_required
def hub():
    return render_template('hub.html', user=current_user)

@app.route('/play/<game_name>')
@login_required
def play_game(game_name):
    valid_games = ['snake', 'pong', 'tetris']
    if game_name not in valid_games:
        return redirect(url_for('hub'))
    
    best = Score.query.filter_by(user_id=current_user.id, game_name=game_name).order_by(Score.score.desc()).first()
    high_score = best.score if best else 0
    return render_template(f'{game_name}.html', user=current_user, high_score=high_score)

@app.route('/api/submit_score', methods=['POST'])
@login_required
def submit_score():
    data = request.json
    game = data.get('game')
    score_val = data.get('score')
    current_best = Score.query.filter_by(user_id=current_user.id, game_name=game).order_by(Score.score.desc()).first()
    
    is_new = False
    if not current_best or score_val > current_best.score:
        new_s = Score(game_name=game, score=score_val, user_id=current_user.id)
        db.session.add(new_s)
        db.session.commit()
        is_new = True
        
    return jsonify({"success": True, "new_record": is_new})

@app.route('/settings', methods=['GET', 'POST'])
@login_required
def settings():
    if request.method == 'POST':
        current_user.snake_color = request.form.get('color')
        db.session.commit()
        return redirect(url_for('hub'))
    return render_template('settings.html', user=current_user)

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        user = User.query.filter_by(username=request.form.get('username')).first()
        if user and user.password == request.form.get('password'):
            login_user(user)
            return redirect(url_for('hub'))
        flash('Invalid Credentials')
    return render_template('login.html')

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        if User.query.filter_by(username=request.form.get('username')).first():
            flash('Username taken')
        else:
            new_user = User(username=request.form.get('username'), password=request.form.get('password'))
            db.session.add(new_user)
            db.session.commit()
            login_user(new_user)
            return redirect(url_for('hub'))
    return render_template('register.html')

@app.route('/logout')
@login_required
def logout():
    logout_user()
    return redirect(url_for('login'))

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    socketio.run(app, host='0.0.0.0', port=5000)